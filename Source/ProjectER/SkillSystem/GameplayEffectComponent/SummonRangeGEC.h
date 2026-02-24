// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectComponent/BaseGEC.h"
#include "SkillSystem/GameplayEffectComponent/BaseGECConfig.h"
#include "SummonRangeGEC.generated.h"

/**
 * 
 */

class ABaseRangeOverlapEffectActor;
class USkillEffectDataAsset;

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API USummonRangeByWorldOriginGECConfig : public UBaseGECConfig
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<ABaseRangeOverlapEffectActor> RangeActorClass;

	UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	float LifeSpan = 1.0f;

	UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	float ZOffset = 0.0f;

	UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	FRotator SpawnRotation = FRotator::ZeroRotator;

	UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	FVector CollisionRadius = FVector(100.0f);

	UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	bool bHitOncePerTarget = true;

	UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<USkillEffectDataAsset>> Applied;
};

UCLASS()
class PROJECTER_API USummonRangeGEC : public UBaseGEC
{
	GENERATED_BODY()
	
public:
	USummonRangeGEC();

	virtual TSubclassOf<UBaseGECConfig> GetRequiredConfigClass() const override;

protected:
	virtual void OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const override;
};
