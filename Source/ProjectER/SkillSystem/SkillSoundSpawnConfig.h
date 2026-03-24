// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/SkillSoundSpawnSettings.h"
#include "GameplayTagContainer.h"
#include "SkillSoundSpawnConfig.generated.h"

class USoundBase;

/**
 * USkillSoundSpawnConfig - UObject-based configuration for sound effects.
 * Mirroring USkillNiagaraSpawnConfig for sound effects.
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API USkillSoundSpawnConfig : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TSoftObjectPtr<USoundBase> Sound;

	UPROPERTY(EditDefaultsOnly, Category = "Sound|Attach")
	bool bAttachToSource = false;

	UPROPERTY(EditDefaultsOnly, Category = "Sound|Attach", meta = (EditCondition = "bAttachToSource"))
	FName SocketOrBoneName = NAME_None;

	UPROPERTY(EditDefaultsOnly, Category = "Sound|Transform")
	bool bUseSourceRotationForLocationOffset = true;

	UPROPERTY(EditDefaultsOnly, Category = "Sound|Transform")
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "Sound|Transform")
	ESoundSpawnRotationMode RotationMode = ESoundSpawnRotationMode::SourceRotationPlusOffset;

	UPROPERTY(EditDefaultsOnly, Category = "Sound|Transform")
	FRotator RotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditDefaultsOnly, Category = "Sound|Parameters")
	float VolumeMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Sound|Parameters")
	float PitchMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Sound|GameplayCue", meta = (Categories = "GameplayCue"))
	FGameplayTag CueTag;

public:
	FSkillSoundSpawnSettings ToSettings() const
	{
		FSkillSoundSpawnSettings S;
		S.Sound = Sound;
		S.bAttachToSource = bAttachToSource;
		S.SocketOrBoneName = SocketOrBoneName;
		S.bUseSourceRotationForLocationOffset = bUseSourceRotationForLocationOffset;
		S.LocationOffset = LocationOffset;
		S.RotationMode = RotationMode;
		S.RotationOffset = RotationOffset;
		S.VolumeMultiplier = VolumeMultiplier;
		S.PitchMultiplier = PitchMultiplier;
		S.CueTag = CueTag;
		return S;
	}
};
