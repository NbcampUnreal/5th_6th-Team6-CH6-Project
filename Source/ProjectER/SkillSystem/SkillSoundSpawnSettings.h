// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SkillSoundSpawnSettings.generated.h"

class USoundBase;

UENUM(BlueprintType)
enum class ESoundSpawnRotationMode : uint8
{
	WorldRotationOffsetOnly UMETA(DisplayName = "World Rotation + Offset"),
	SourceRotationPlusOffset UMETA(DisplayName = "Source Rotation + Offset"),
	LookAtTargetPlusOffset UMETA(DisplayName = "LookAt Target + Offset")
};

USTRUCT(BlueprintType)
struct FSkillSoundSpawnSettings
{
	GENERATED_BODY()

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
};
