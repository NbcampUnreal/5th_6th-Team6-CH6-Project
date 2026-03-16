#include "SkillSystem/SkillNiagaraSpawnHelper.h"
#include "Components/SkeletalMeshComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "SkillSystem/SkillNiagaraSpawnSettings.h"

namespace
{
	USceneComponent* ResolveAttachComponent(const AActor* SourceActor, const FSkillNiagaraSpawnSettings& Settings)
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

	FRotator CalculateWorldRotation(const FSkillNiagaraSpawnSettings& Settings, const FTransform& SourceTransform, const FVector* OptionalLookAtTarget)
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

	FRotator CalculateRelativeRotation(const FSkillNiagaraSpawnSettings& Settings, const FTransform& ParentTransform, const FVector* OptionalLookAtTarget)
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

void SkillNiagaraSpawnHelper::SpawnNiagaraBySettings(UWorld* World, const FSkillNiagaraSpawnSettings& Settings, const FTransform& SourceTransform, const AActor* SourceActor, const FVector* OptionalLookAtTarget)
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

	if (Settings.bAttachToSource && IsValid(SourceActor))
	{
		USceneComponent* AttachComponent = ResolveAttachComponent(SourceActor, Settings);
		if (!IsValid(AttachComponent))
		{
			return;
		}

		const FTransform ParentTransform = AttachComponent->GetSocketTransform(Settings.SocketOrBoneName);
		
		// EAttachLocation::KeepRelativeOffset를 사용하기 때문에, 지정할 회전값은 반드시 '상대 회전(Relative Rotation)' 이어야 합니다.
		// 그래서 CalculateRelativeRotation 함수를 호출하여 완벽히 분리된 로컬 회전값을 도출합니다.
		const FRotator RelativeAttachRotation = CalculateRelativeRotation(Settings, ParentTransform, OptionalLookAtTarget);

		//UE_LOG(LogTemp, Warning, TEXT("SpawnNiagaraBySettings::AppliedRelative: %s"), *RelativeAttachRotation.ToString());
		ResultNC = UNiagaraFunctionLibrary::SpawnSystemAttached(LoadedNiagaraSystem, AttachComponent, Settings.SocketOrBoneName, Settings.LocationOffset, RelativeAttachRotation, Settings.Scale, EAttachLocation::KeepRelativeOffset, true, ENCPoolMethod::None, true, false);
	}
	else {
		const FVector SourceLocation = SourceTransform.GetLocation();
		const FQuat SourceRotation = SourceTransform.GetRotation();
		const FVector WorldLocationOffset = Settings.bUseSourceRotationForLocationOffset
			? SourceRotation.RotateVector(Settings.LocationOffset)
			: Settings.LocationOffset;
		const FVector SpawnLocation = SourceLocation + WorldLocationOffset;
		const FRotator SpawnRotation = CalculateWorldRotation(Settings, SourceTransform, OptionalLookAtTarget);
		ResultNC = UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, LoadedNiagaraSystem, SpawnLocation, SpawnRotation, Settings.Scale, true, true);
	}
	
	if (!IsValid(ResultNC)) {
		UE_LOG(LogTemp, Warning, TEXT("ResultNC is Null, Niagara is Not Spawn"));
		return;
	}
}