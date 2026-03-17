// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectComponent/SummonRangeBaseGEC.h"
#include "SummonPeriodicPoolGEC.generated.h"

UCLASS()
class PROJECTER_API USummonPeriodicPoolConfig : public USummonRangeBaseConfig
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Periodic")
    float Period = 1.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Periodic")
    bool bApplyImmediately = true;
};

UCLASS()
class PROJECTER_API USummonPeriodicPoolGEC : public USummonRangeBaseGEC
{
    GENERATED_BODY()

public:
    virtual TSubclassOf<UBaseGECConfig> GetRequiredConfigClass() const override;

protected:
    virtual void InitializeRangeActor(ABaseRangeOverlapEffectActor* RangeActor, const USummonRangeBaseConfig* Config, AActor* Instigator, const FGameplayEffectContextHandle& Context, const FGameplayCueParameters& HitTargetCueParameters) const override;
};
