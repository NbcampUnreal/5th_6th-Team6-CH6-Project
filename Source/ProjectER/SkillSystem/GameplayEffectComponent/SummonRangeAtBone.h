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
    /* --- 기초 설정 (Base) --- */
    UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Base")
    TSubclassOf<ABaseRangeOverlapEffectActor> RangeActorClass;

    UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Base")
    float LifeSpan = 1.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Base")
    FName BoneName;

    UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Base")
    FVector LocationOffset;

    UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Base")
    FVector CollisionRadius = FVector(100.0f);

    /* --- 회전 설정 (Rotation) --- */
    UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Rotation")
    bool bUseInstigatorRotation = false;

    UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Rotation")
    bool bSpawnZeroRotation = false;

    UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Rotation", meta = (EditCondition = "!bUseInstigatorRotation && !bSpawnZeroRotation"))
    FRotator RotationOffset = FRotator::ZeroRotator;

    /* --- 지형 안착 설정 (Snap) --- */
    UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Snap")
    bool bSnapToGround = true;

    UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Snap", meta = (EditCondition = "bSnapToGround"))
    float GroundOffset = 2.0f;

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
class PROJECTER_API USummonRangeAtBone : public UBaseGEC
{
	GENERATED_BODY()
	
public:
	USummonRangeAtBone();

	virtual TSubclassOf<UBaseGECConfig> GetRequiredConfigClass() const override;

protected:
	virtual void OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const override;
	const USummonRangeByBoneGECConfig* GetSpawnConfig(const FGameplayEffectSpec& GESpec) const;
	FTransform CalculateSpawnLocation(const AActor* Instigator, const USummonRangeByBoneGECConfig* Config) const;
	void InitializeRangeActor(ABaseRangeOverlapEffectActor* RangeActor, const USummonRangeByBoneGECConfig* Config, AActor* Causer, const FGameplayEffectContextHandle& Context) const;
};
