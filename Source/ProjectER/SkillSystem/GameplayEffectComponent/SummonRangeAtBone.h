// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectComponent/BaseGEC.h"
#include "SkillSystem/GameplayEffectComponent/BaseGECConfig.h"
#include "SummonRangeAtBone.generated.h"

/**
 * 
 */
class ABaseRangeOverlapEffectActor;
class USkillEffectDataAsset;
struct FGameplayEffectContextHandle;

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API USummonRangeByBoneGECConfig : public UBaseGECConfig
{
	GENERATED_BODY()

public:
	virtual FText BuildTooltipDescription(float InLevel) const override;

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

	UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	FName BoneName;

	UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	FVector LocationOffset;
};

UCLASS()
class PROJECTER_API USummonRangeAtBone : public UBaseGEC
{
	GENERATED_BODY()
	
public:
	USummonRangeAtBone();

	virtual TSubclassOf<UBaseGECConfig> GetRequiredConfigClass() const override;

protected:
	virtual void OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const override;
	const USummonRangeByBoneGECConfig* GetSpawnConfig(const FGameplayEffectSpec& GESpec) const;
	FVector CalculateSpawnLocation(const AActor* Instigator, const USummonRangeByBoneGECConfig* Config) const;
	void InitializeRangeActor(ABaseRangeOverlapEffectActor* RangeActor, const USummonRangeByBoneGECConfig* Config, AActor* Causer, const FGameplayEffectContextHandle& Context) const;
};
