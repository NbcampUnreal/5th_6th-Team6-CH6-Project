#include "SkillSystem/SkillSoundSpawnHelper.h"
#include "SkillSystem/SkillSoundSpawnSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Sound/SoundBase.h"

namespace
{
	USceneComponent* ResolveSoundAttachComponent(const AActor* SourceActor, const FSkillSoundSpawnSettings& Settings)
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

	FRotator CalculateSoundWorldRotation(const FSkillSoundSpawnSettings& Settings, const FTransform& SourceTransform, const FVector* OptionalLookAtTarget)
	{
		switch (Settings.RotationMode)
		{
		case ESoundSpawnRotationMode::WorldRotationOffsetOnly:
			return Settings.RotationOffset;

		case ESoundSpawnRotationMode::LookAtTargetPlusOffset:
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

		case ESoundSpawnRotationMode::SourceRotationPlusOffset:
		default:
			return SourceTransform.GetRotation().Rotator() + Settings.RotationOffset;
		}
	}

	FRotator CalculateSoundRelativeRotation(const FSkillSoundSpawnSettings& Settings, const FTransform& ParentTransform, const FVector* OptionalLookAtTarget)
	{
		switch (Settings.RotationMode)
		{
		case ESoundSpawnRotationMode::WorldRotationOffsetOnly:
			return (ParentTransform.GetRotation().Inverse() * Settings.RotationOffset.Quaternion()).Rotator();

		case ESoundSpawnRotationMode::LookAtTargetPlusOffset:
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

		case ESoundSpawnRotationMode::SourceRotationPlusOffset:
		default:
			return Settings.RotationOffset;
		}
	}
}

void SkillSoundSpawnHelper::PlaySoundBySettings(UWorld* World, const FSkillSoundSpawnSettings& Settings, const FTransform& SourceTransform, const AActor* SourceActor, const FVector* OptionalLookAtTarget, USceneComponent* AttachTarget)
{
	if (!IsValid(World) || Settings.Sound.IsNull())
	{
		return;
	}

	USoundBase* LoadedSound = Settings.Sound.LoadSynchronous();
	if (!IsValid(LoadedSound))
	{
		return;
	}

	if (Settings.bAttachToSource && (IsValid(SourceActor) || IsValid(AttachTarget)))
	{
		USceneComponent* FinalAttachComponent = IsValid(AttachTarget) ? AttachTarget : ResolveSoundAttachComponent(SourceActor, Settings);
		if (IsValid(FinalAttachComponent))
		{
			const FTransform ParentTransform = FinalAttachComponent->GetSocketTransform(Settings.SocketOrBoneName);
			const FRotator RelativeAttachRotation = CalculateSoundRelativeRotation(Settings, ParentTransform, OptionalLookAtTarget);
			
			UGameplayStatics::SpawnSoundAttached(LoadedSound, FinalAttachComponent, Settings.SocketOrBoneName, Settings.LocationOffset, RelativeAttachRotation, EAttachLocation::KeepRelativeOffset, false, Settings.VolumeMultiplier, Settings.PitchMultiplier);
			return;
		}
	}

	// At Location
	const FVector SourceLocation = SourceTransform.GetLocation();
	const FQuat SourceRotation = SourceTransform.GetRotation();
	const FVector WorldLocationOffset = Settings.bUseSourceRotationForLocationOffset
		? SourceRotation.RotateVector(Settings.LocationOffset)
		: Settings.LocationOffset;
	const FVector SpawnLocation = SourceLocation + WorldLocationOffset;
	const FRotator SpawnRotation = CalculateSoundWorldRotation(Settings, SourceTransform, OptionalLookAtTarget);

	UGameplayStatics::PlaySoundAtLocation(World, LoadedSound, SpawnLocation, SpawnRotation, Settings.VolumeMultiplier, Settings.PitchMultiplier);
}
