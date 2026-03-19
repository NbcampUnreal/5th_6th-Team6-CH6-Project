// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectComponent/SummonRangeGEC.h"
#include "SummonPeriodicPoolGEC.generated.h"

UENUM(BlueprintType)
enum class ESummonOriginType : uint8
{
    Context,
    InstigatorBone
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API USummonPeriodicPoolConfig : public USummonRangeByWorldOriginGECConfig
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Periodic")
    float Period = 1.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Periodic")
    bool bApplyImmediately = true;

    UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Periodic")
    ESummonOriginType OriginType = ESummonOriginType::Context;

    UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Periodic", meta = (EditCondition = "OriginType == ESummonOriginType::InstigatorBone"))
    FName SummonBoneName;
};

UCLASS()
class PROJECTER_API USummonPeriodicPoolGEC : public USummonRangeGEC
{
    GENERATED_BODY()

public:
    USummonPeriodicPoolGEC();
    virtual TSubclassOf<UBaseGECConfig> GetRequiredConfigClass() const override;

protected:
    virtual FTransform CalculateSpawnTransform(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const AActor* TargetActor) const override;
    virtual void InitializeRangeActor(ABaseRangeOverlapEffectActor* RangeActor, const USummonRangeBaseConfig* Config, AActor* Instigator, const FGameplayEffectContextHandle& Context, const FGameplayCueParameters& HitTargetCueParameters) const override;
};
