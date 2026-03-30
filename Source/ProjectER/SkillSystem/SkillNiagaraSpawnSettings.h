// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SkillNiagaraSpawnSettings.generated.h"
/**
 * 
 */

class AActor;
class UNiagaraSystem;
class USceneComponent;

UENUM(BlueprintType)
enum class ENiagaraSpawnRotationMode : uint8
{
	WorldRotationOffsetOnly UMETA(DisplayName = "World Rotation + Offset"),
	SourceRotationPlusOffset UMETA(DisplayName = "Source Rotation + Offset"),
	LookAtTargetPlusOffset UMETA(DisplayName = "LookAt Target + Offset")
};

USTRUCT(BlueprintType)
struct FSkillNiagaraSpawnSettings
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Niagara")
	TSoftObjectPtr<UNiagaraSystem> NiagaraSystem;

	UPROPERTY(EditDefaultsOnly, Category = "Niagara|Attach")
	bool bAttachToSource = false;

	UPROPERTY(EditDefaultsOnly, Category = "Niagara|Attach", meta = (EditCondition = "bAttachToSource"))
	FName SocketOrBoneName = NAME_None;

	UPROPERTY(EditDefaultsOnly, Category = "Niagara|Transform")
	bool bUseSourceRotationForLocationOffset = true;

	UPROPERTY(EditDefaultsOnly, Category = "Niagara|Transform")
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "Niagara|Transform")
	ENiagaraSpawnRotationMode RotationMode = ENiagaraSpawnRotationMode::SourceRotationPlusOffset;

	UPROPERTY(EditDefaultsOnly, Category = "Niagara|Transform")
	FRotator RotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditDefaultsOnly, Category = "Niagara|GameplayCue", meta = (Categories = "GameplayCue"))
	FGameplayTag CueTag;

	UPROPERTY(EditDefaultsOnly, Category = "Niagara|Parameters")
	TMap<FName, float> FloatParameters;

	UPROPERTY(EditDefaultsOnly, Category = "Niagara|Parameters")
	TMap<FName, FVector> VectorParameters;

	UPROPERTY(EditDefaultsOnly, Category = "Niagara|Parameters")
	TMap<FName, FLinearColor> ColorParameters;
};