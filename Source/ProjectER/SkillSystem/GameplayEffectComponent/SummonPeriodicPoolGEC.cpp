// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/GameplayEffectComponent/SummonPeriodicPoolGEC.h"
#include "SkillSystem/GameplayEffectComponent/SummonPeriodicPoolConfig.h"
#include "SkillSystem/Actor/BaseRangeOverlapEffectActor/BaseRangeOverlapEffectActor.h"
#include "SkillSystem/Component/AreaPeriodicEffectComponent.h"

TSubclassOf<UBaseGECConfig> USummonPeriodicPoolGEC::GetRequiredConfigClass() const
{
    return USummonPeriodicPoolConfig::StaticClass();
}

void USummonPeriodicPoolGEC::InitializeRangeActor(ABaseRangeOverlapEffectActor* RangeActor, const USummonRangeBaseConfig* Config, AActor* Instigator, const FGameplayEffectContextHandle& Context, const FGameplayCueParameters& HitTargetCueParameters) const
{
    // 부모의 초기화 로직 (Effect Specs 설정 등) 실행
    Super::InitializeRangeActor(RangeActor, Config, Instigator, Context, HitTargetCueParameters);

    if (IsValid(RangeActor) && IsValid(PeriodicConfig))
    {
        // 1. AreaPeriodicEffectComponent 동적 생성
        UAreaPeriodicEffectComponent* PeriodicComp = NewObject<UAreaPeriodicEffectComponent>(RangeActor, UAreaPeriodicEffectComponent::StaticClass(), TEXT("DynamicAreaPeriodicEffect"));
        if (IsValid(PeriodicComp))
        {
            PeriodicComp->CreationMethod = EComponentCreationMethod::Instance;

            // 2. 컴포넌트 등록 및 액터 할당
            PeriodicComp->RegisterComponent();
            RangeActor->AddInstanceComponent(PeriodicComp);
            RangeActor->SetAreaPeriodicComponent(PeriodicComp);

            // 3. 주기적 효과 설정 (실행은 액터의 BeginPlay에서 담당)
            PeriodicComp->SetupPeriodicEffect(PeriodicConfig->Period, PeriodicConfig->bApplyImmediately);
        }
    }
}
