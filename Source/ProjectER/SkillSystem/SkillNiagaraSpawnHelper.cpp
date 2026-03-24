#include "SkillSystem/SkillNiagaraSpawnHelper.h"
#include "Components/SkeletalMeshComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "SkillSystem/SkillNiagaraSpawnSettings.h"
#include "Engine/World.h"

namespace
{
	USceneComponent* ResolveNiagaraAttachComponent(const AActor* SourceActor, const FSkillNiagaraSpawnSettings& Settings)
	{
		if (!IsValid(SourceActor))
		{
			return nullptr;
		}

		if (Settings.SocketOrBoneName != NAME_None)
		{
			if (USkeletalMeshComponent* SkeletalMeshComponent = SourceActor->FindComponentByClass<USkeletalMeshComponent>())
			{
				if (SkeletalMeshComponent->DoesSocketExist(Settings.SocketOrBoneName))
				{
					return SkeletalMeshComponent;
				}
			}
		}

		return SourceActor->GetRootComponent();
	}

	FRotator CalculateNiagaraWorldRotation(const FSkillNiagaraSpawnSettings& Settings, const FTransform& SourceTransform, const FVector* OptionalLookAtTarget)
	{
		switch (Settings.RotationMode)
		{
		case ENiagaraSpawnRotationMode::WorldRotationOffsetOnly:
			return Settings.RotationOffset;

		case ENiagaraSpawnRotationMode::LookAtTargetPlusOffset:
			if (OptionalLookAtTarget != nullptr)
			{
				const FVector SourceLocation = SourceTransform.GetLocation();
				const FVector LookDirection = *OptionalLookAtTarget - SourceLocation;
				if (!LookDirection.IsNearlyZero())
				{
					return FRotationMatrix::MakeFromX(LookDirection).Rotator() + Settings.RotationOffset;
				}
			}
			return SourceTransform.GetRotation().Rotator() + Settings.RotationOffset;

		case ENiagaraSpawnRotationMode::SourceRotationPlusOffset:
		default:
			return SourceTransform.GetRotation().Rotator() + Settings.RotationOffset;
		}
	}

	FRotator CalculateNiagaraRelativeRotation(const FSkillNiagaraSpawnSettings& Settings, const FTransform& ParentTransform, const FVector* OptionalLookAtTarget)
	{
		switch (Settings.RotationMode)
		{
		case ENiagaraSpawnRotationMode::WorldRotationOffsetOnly:
			// 부모의 회전을 취소하고 순수 월드 오프셋만 바라보게 함
			return (ParentTransform.GetRotation().Inverse() * Settings.RotationOffset.Quaternion()).Rotator();

		case ENiagaraSpawnRotationMode::LookAtTargetPlusOffset:
			if (OptionalLookAtTarget != nullptr)
			{
				const FVector ParentLocation = ParentTransform.GetLocation();
				const FVector LookDirection = *OptionalLookAtTarget - ParentLocation;
				if (!LookDirection.IsNearlyZero())
				{
					const FRotator TargetWorldRot = FRotationMatrix::MakeFromX(LookDirection).Rotator() + Settings.RotationOffset;
					return (ParentTransform.GetRotation().Inverse() * TargetWorldRot.Quaternion()).Rotator();
				}
			}
			return Settings.RotationOffset;

		case ENiagaraSpawnRotationMode::SourceRotationPlusOffset:
		default:
			// 부모(Source)를 그대로 따라가면 되므로 순수 로컬 오프셋만 반환
			return Settings.RotationOffset;
		}
	}
}

void SkillNiagaraSpawnHelper::SpawnNiagaraBySettings(UWorld* World, const FSkillNiagaraSpawnSettings& Settings, const FTransform& SourceTransform, const AActor* SourceActor, const FVector* OptionalLookAtTarget, USceneComponent* AttachTarget)
{
	UNiagaraComponent* ResultNC = nullptr;

	if (!IsValid(World) || Settings.NiagaraSystem.IsNull())
	{
		return;
	}

	UNiagaraSystem* LoadedNiagaraSystem = Settings.NiagaraSystem.LoadSynchronous();
	if (!IsValid(LoadedNiagaraSystem))
	{
		return;
	}

	if (Settings.bAttachToSource && (IsValid(SourceActor) || IsValid(AttachTarget)))
	{
		USceneComponent* FinalAttachComponent = IsValid(AttachTarget) ? AttachTarget : ResolveNiagaraAttachComponent(SourceActor, Settings);
		if (!IsValid(FinalAttachComponent))
		{
			return;
		}

		const FTransform ParentTransform = FinalAttachComponent->GetSocketTransform(Settings.SocketOrBoneName);
		const FRotator RelativeAttachRotation = CalculateNiagaraRelativeRotation(Settings, ParentTransform, OptionalLookAtTarget);
		ResultNC = UNiagaraFunctionLibrary::SpawnSystemAttached(LoadedNiagaraSystem, FinalAttachComponent, Settings.SocketOrBoneName, Settings.LocationOffset, RelativeAttachRotation, Settings.Scale, EAttachLocation::KeepRelativeOffset, true, ENCPoolMethod::None, false, false);
	}
	else {
		const FVector SourceLocation = SourceTransform.GetLocation();
		const FQuat SourceRotation = SourceTransform.GetRotation();
		const FVector WorldLocationOffset = Settings.bUseSourceRotationForLocationOffset
			? SourceRotation.RotateVector(Settings.LocationOffset)
			: Settings.LocationOffset;
		const FVector SpawnLocation = SourceLocation + WorldLocationOffset;
		const FRotator SpawnRotation = CalculateNiagaraWorldRotation(Settings, SourceTransform, OptionalLookAtTarget);
		ResultNC = UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, LoadedNiagaraSystem, SpawnLocation, SpawnRotation, Settings.Scale, true, false);
	}
	
	if (!IsValid(ResultNC)) {
		UE_LOG(LogTemp, Warning, TEXT("ResultNC is Null, Niagara is Not Spawn"));
		return;
	}
	else{
		for (const auto& Pair : Settings.FloatParameters)
		{
			ResultNC->SetVariableFloat(Pair.Key, Pair.Value);
		}
		for (const auto& Pair : Settings.VectorParameters)
		{
			ResultNC->SetVariableVec3(Pair.Key, Pair.Value);
		}
		for (const auto& Pair : Settings.ColorParameters)
		{
			ResultNC->SetVariableLinearColor(Pair.Key, Pair.Value);
		}

		ResultNC->Activate();
	}
}