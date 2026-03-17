// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/GameplayEffectComponent/SummonPeriodicPoolGEC.h"
#include "SkillSystem/Actor/PeriodicOverlapPoolActor/PeriodicOverlapPoolActor.h"

TSubclassOf<UBaseGECConfig> USummonPeriodicPoolGEC::GetRequiredConfigClass() const
{
    return USummonPeriodicPoolConfig::StaticClass();
}

void USummonPeriodicPoolGEC::InitializeRangeActor(ABaseRangeOverlapEffectActor* RangeActor, const USummonRangeBaseConfig* Config, AActor* Instigator, const FGameplayEffectContextHandle& Context, const FGameplayCueParameters& HitTargetCueParameters) const
{
    // 부모의 초기화 로직 (Effect Specs 설정 등) 실행
    Super::InitializeRangeActor(RangeActor, Config, Instigator, Context, HitTargetCueParameters);

    // 주기적 효과 데이터 설정
    if (APeriodicOverlapPoolActor* PeriodicActor = Cast<APeriodicOverlapPoolActor>(RangeActor))
    {
        if (const USummonPeriodicPoolConfig* PeriodicConfig = Cast<USummonPeriodicPoolConfig>(Config))
        {
            PeriodicActor->InitializePeriodicData(PeriodicConfig->Period, PeriodicConfig->bApplyImmediately);
        }
    }
}
