// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/GameplayEffect/BaseGameplayEffect.h"
#include "GameplayEffect.h"
#include "SkillSystem/GameplayEffectExecutionCalculation/BaseExecutionCalculation.h"
#include "SkillSystem/GameplayModMagnitudeCalculation/BaseModMagnitudeCalculation.h"

UBaseGameplayEffect::UBaseGameplayEffect() {
}

#if WITH_EDITOR
void UBaseGameplayEffect::PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) {
    Super::PostEditChangeProperty(PropertyChangedEvent);

    // 1. Executions 고정: 하나라도 존재할 때만 클래스를 강제함 (자동 추가 X)
    for (FGameplayEffectExecutionDefinition &ExecDef : Executions) {
        if (ExecDef.CalculationClass == nullptr ||
            !ExecDef.CalculationClass->IsChildOf(UBaseExecutionCalculation::StaticClass())) 
        {
            ExecDef.CalculationClass = UBaseExecutionCalculation::StaticClass();
        }
    }

    // 2. Modifiers 고정: 무조건 BaseModMagnitudeCalculation으로 강제함
    for (FGameplayModifierInfo& ModInfo : Modifiers)
    {
        // 현재 클래스가 이미 BaseModMagnitudeCalculation 계열인지 확인
        if (ModInfo.ModifierMagnitude.GetMagnitudeCalculationType() != EGameplayEffectMagnitudeCalculation::CustomCalculationClass ||
            ModInfo.ModifierMagnitude.GetCustomMagnitudeCalculationClass() == nullptr ||
            !ModInfo.ModifierMagnitude.GetCustomMagnitudeCalculationClass()->IsChildOf(UBaseModMagnitudeCalculation::StaticClass()))
        {
            // 사용자의 환경(UE 5.7 추정)에 맞는 구조체 및 멤버 변수명을 사용합니다.
            FCustomCalculationBasedFloat NewCustomCalc;
            NewCustomCalc.CalculationClassMagnitude = UBaseModMagnitudeCalculation::StaticClass();

            // 생성자를 통해 매그니튜드 타입(CustomCalculationClass)과 클래스를 동시에 강제 설정합니다.
            ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(NewCustomCalc);
        }
    }

    // 변경사항이 즉시 에디터에 반영되도록 합니다.
    this->MarkPackageDirty();
}
#endif
