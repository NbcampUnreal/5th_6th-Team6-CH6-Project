// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplayModMagnitudeCalculation/BaseMMC.h"
#include "SkillSystem/GameplyeEffect/SkillEffectDataAsset.h"

UBaseMMC::UBaseMMC()
{
}

float UBaseMMC::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
    FGameplayEffectContextHandle ContextHandle = Spec.GetContext();
    ContextHandle.GetSourceObject();

    USkillEffectDataAsset* SkillEffectDataAsset = Cast<USkillEffectDataAsset>(ContextHandle.GetSourceObject());
    if (!SkillEffectDataAsset) UE_LOG(LogTemp, Log, TEXT("UBaseMMC:: SkillEffectDataAsset is Null"));

    //SkillEffectDataAsset->Data


    // 1. GA가 'Data.IncomingStat'이라는 이름표로 보낸 수치를 꺼냅니다.
    //FGameplayTag StatTag = FGameplayTag::RequestGameplayTag(FName("Skill.Data.IncomingStatus"));

    /*FGameplayEffectContextHandle ContextHandle = Spec.GetContext();
    AActor* Instigator = ContextHandle.GetInstigator();*/

    // SetByCaller로 전달된 값을 가져옵니다. (찾지 못하면 0.0f 리턴)
    //float PassedStatValue = Spec.GetSetByCallerMagnitude(StatTag, false, 0.f);

    // 2. 뽑아낸 스탯 값(예: 지능 150)만 그대로 리턴합니다.
    // 이후 계산(스탯 * 계수 + 기본값)은 엔진이 GE 설정을 보고 수행합니다.
    // 0 이하값은 반환금지
    //return FMath::Max<float>(PassedStatValue, 0.f);
    return 0;
}
