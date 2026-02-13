// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplyeEffect/BaseGameplayEffect.h"
#include "SkillSystem/GameplayEffectExecutionCalculation/BaseExecutionCalculation.h"
#include "GameplayEffect.h"

UBaseGameplayEffect::UBaseGameplayEffect()
{
    FGameplayEffectExecutionDefinition NewExec;
    NewExec.CalculationClass = UBaseExecutionCalculation::StaticClass();
    Executions.Add(NewExec);
}

void UBaseGameplayEffect::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    //// 1. 공용으로 쓸 설정 데이터 준비
    //FCustomCalculationBasedFloat CustomMagnitude;
    //CustomMagnitude.CalculationClassMagnitude = UBaseMMC::StaticClass();

    //for (FGameplayModifierInfo& Modifier : Modifiers)
    //{
    //    // 현재 설정된 클래스 확인
    //    TSubclassOf<UGameplayModMagnitudeCalculation> CurrentMMC = Modifier.ModifierMagnitude.GetCustomMagnitudeCalculationClass();

    //    // 타입이 MMC가 아니거나, 클래스가 부적절한 경우 실행
    //    bool bIsWrongType = (Modifier.ModifierMagnitude.GetMagnitudeCalculationType() != EGameplayEffectMagnitudeCalculation::CustomCalculationClass);
    //    bool bIsWrongClass = (CurrentMMC == nullptr || !CurrentMMC->IsChildOf(UBaseMMC::StaticClass()));

    //    if (bIsWrongType || bIsWrongClass)
    //    {
    //        // 강제로 MMC 타입 및 UBaseMMC 클래스로 고정
    //        Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(CustomMagnitude);
    //    }
    //}

    if (Executions.Num() == 0)
    {
        FGameplayEffectExecutionDefinition NewExec;
        NewExec.CalculationClass = UBaseExecutionCalculation::StaticClass();
        Executions.Add(NewExec);
    }
    else
    {
        // 이미 있다면, 첫 번째(혹은 전체) Execution의 클래스가 Base를 상속받았는지 확인
        for (FGameplayEffectExecutionDefinition& ExecDef : Executions)
        {
            if (ExecDef.CalculationClass == nullptr || !ExecDef.CalculationClass->IsChildOf(UBaseExecutionCalculation::StaticClass()))
            {
                ExecDef.CalculationClass = UBaseExecutionCalculation::StaticClass();
            }
        }
    }
}
