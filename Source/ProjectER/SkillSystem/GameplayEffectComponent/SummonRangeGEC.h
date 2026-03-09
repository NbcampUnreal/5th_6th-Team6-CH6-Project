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
struct FGameplayEffectContextHandle;

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API USummonRangeByWorldOriginGECConfig : public UBaseGECConfig
{
	GENERATED_BODY()

public:
	virtual FText BuildTooltipDescription(float InLevel) const override;

public:
	/* --- 기초 설정 (Base) --- */
	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Base")
	TSubclassOf<ABaseRangeOverlapEffectActor> RangeActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Base")
	float LifeSpan = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Base")
	FVector CollisionRadius = FVector(100.0f);

	/* --- 회전 설정 (Rotation) --- */
	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Rotation")
	bool bLookAtTargetLocation = false;

	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Rotation")
	bool bSpawnZeroRotation = false;

	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Rotation", meta = (EditCondition = "!bLookAtTargetLocation && !bSpawnZeroRotation"))
	FRotator RotationOffset = FRotator::ZeroRotator;

	/* --- 지형 안착 설정 (Snap) --- */
	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Snap")
	bool bSnapToGround = true;

	/** 바닥으로부터 최종적으로 띄울 높이 */
	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Snap", meta = (EditCondition = "bSnapToGround"))
	float FloatingHeight = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Snap", meta = (EditCondition = "bSnapToGround"))
	bool bUseBoxExtentOffset = true;

	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Snap", meta = (EditCondition = "bSnapToGround"))
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

	/* --- 효과 설정 (Effect) --- */
	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Effect")
	bool bHitOncePerTarget = true;

	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Effect")
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
	const USummonRangeByWorldOriginGECConfig* GetSpawnConfig(const FGameplayEffectSpec& GESpec) const;
	FTransform CalculateSpawnTransform(const FGameplayEffectSpec& GESpec, const USummonRangeByWorldOriginGECConfig* Config) const;
	void InitializeRangeActor(ABaseRangeOverlapEffectActor* RangeActor, const USummonRangeByWorldOriginGECConfig* Config, AActor* Causer, const FGameplayEffectContextHandle& Context) const;
};
