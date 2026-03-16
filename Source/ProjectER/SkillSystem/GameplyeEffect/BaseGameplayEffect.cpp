// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/GameplyeEffect/BaseGameplayEffect.h"
#include "GameplayEffect.h"
#include "SkillSystem/GameplayEffectExecutionCalculation/BaseExecutionCalculation.h"


UBaseGameplayEffect::UBaseGameplayEffect() {
  FGameplayEffectExecutionDefinition NewExec;
  NewExec.CalculationClass = UBaseExecutionCalculation::StaticClass();
  Executions.Add(NewExec);
}

#if WITH_EDITOR
void UBaseGameplayEffect::PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) {
  Super::PostEditChangeProperty(PropertyChangedEvent);
  if (Executions.Num() == 0) {
    FGameplayEffectExecutionDefinition NewExec;
    NewExec.CalculationClass = UBaseExecutionCalculation::StaticClass();
    Executions.Add(NewExec);
  } else {
    // 이미 있다면, 첫 번째(혹은 전체) Execution의 클래스가 Base를 상속받았는지
    // 확인
    for (FGameplayEffectExecutionDefinition &ExecDef : Executions) {
      if (ExecDef.CalculationClass == nullptr ||
          !ExecDef.CalculationClass->IsChildOf(
              UBaseExecutionCalculation::StaticClass())) {
        ExecDef.CalculationClass = UBaseExecutionCalculation::StaticClass();
      }
    }
  }
}
#endif
