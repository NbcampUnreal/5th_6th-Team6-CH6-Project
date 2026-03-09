#include "SkillSystem/SkillNiagaraSpawnHelper.h"
#include "Components/SkeletalMeshComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
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

	FRotator CalculateSpawnRotation(const FSkillNiagaraSpawnSettings& Settings, const FTransform& SourceTransform, const FVector* OptionalLookAtTarget)
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
}

void SkillNiagaraSpawnHelper::SpawnNiagaraBySettings(UWorld* World, const FSkillNiagaraSpawnSettings& Settings, const FTransform& SourceTransform, const AActor* SourceActor, const FVector* OptionalLookAtTarget)
{
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

		const FRotator AttachRotation = CalculateSpawnRotation(Settings, SourceTransform, OptionalLookAtTarget);
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			LoadedNiagaraSystem,             // SystemTemplate
			AttachComponent,                 // AttachToComponent
			Settings.SocketOrBoneName,                // AttachPointName
			Settings.LocationOffset,                  // Location
			AttachRotation,                  // Rotation
			Settings.Scale,                           // Scale (FVector 타입 인자 필수!)
			EAttachLocation::KeepRelativeOffset, // LocationType
			true,                            // bAutoDestroy
			ENCPoolMethod::None,             // PoolingMethod
			true,                            // bAutoActivate (bool 값을 넣어야 함)
			false                             // bPreCullCheck
		);
		return;
	}

	const FVector SourceLocation = SourceTransform.GetLocation();
	const FQuat SourceRotation = SourceTransform.GetRotation();
	const FVector WorldLocationOffset = Settings.bUseSourceRotationForLocationOffset
		? SourceRotation.RotateVector(Settings.LocationOffset)
		: Settings.LocationOffset;
	const FVector SpawnLocation = SourceLocation + WorldLocationOffset;
	const FRotator SpawnRotation = CalculateSpawnRotation(Settings, SourceTransform, OptionalLookAtTarget);

	UNiagaraComponent* qwe =UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, LoadedNiagaraSystem, SpawnLocation, SpawnRotation, Settings.Scale, true, true);
	if (qwe)
	{
		UE_LOG(LogTemp, Warning, TEXT("qwe is not Null"));
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("qwe is Null"));
	}

}