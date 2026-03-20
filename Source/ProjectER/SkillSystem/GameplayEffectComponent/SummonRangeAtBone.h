// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectComponent/SummonRangeBaseGEC.h"
#include "SummonRangeAtBone.generated.h"

/**
 * 
 */
class ABaseRangeOverlapEffectActor;
class USkillEffectDataAsset;
struct FGameplayEffectContextHandle;
struct FGameplayCueParameters;

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API USummonRangeByBoneGECConfig : public USummonRangeBaseConfig
{
    GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Base")
	FName BoneName;
	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Rotation")
	bool bUseInstigatorRotation = false;
};

UCLASS()
class PROJECTER_API USummonRangeAtBone : public USummonRangeBaseGEC
{
	GENERATED_BODY()
	
public:
	USummonRangeAtBone();

	virtual TSubclassOf<UBaseGECConfig> GetRequiredConfigClass() const override;

protected:
	virtual bool ShouldProcessOnInstigator(const AActor* Instigator) const override;
	virtual FTransform CalculateOriginTransform(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const AActor* TargetActor) const override;
};
