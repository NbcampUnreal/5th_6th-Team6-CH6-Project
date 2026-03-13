// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectComponent/SummonRangeBaseGEC.h"
#include "SummonRangeGEC.generated.h"

/**
 * 
 */

class ABaseRangeOverlapEffectActor;
class USkillEffectDataAsset;
struct FGameplayEffectContextHandle;
struct FGameplayCueParameters;

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API USummonRangeByWorldOriginGECConfig : public USummonRangeBaseConfig
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Rotation")
	bool bLookAtTargetLocation = false;
};

UCLASS()
class PROJECTER_API USummonRangeGEC : public USummonRangeBaseGEC
{
	GENERATED_BODY()
	
public:
	USummonRangeGEC();

	virtual TSubclassOf<UBaseGECConfig> GetRequiredConfigClass() const override;

protected:
	virtual FTransform CalculateSpawnTransform(const FGameplayEffectSpec& GESpec, const AActor* Instigator) const override;
	FVector GetAnyLocation(const FGameplayEffectContextHandle& ContextHandle) const;
};
