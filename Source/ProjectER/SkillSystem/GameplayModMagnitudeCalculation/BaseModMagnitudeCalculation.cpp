// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplayModMagnitudeCalculation/BaseModMagnitudeCalculation.h"
#include "SkillSystem/SkillDataAsset.h"
#include "SkillSystem/GameplayEffect/SkillEffectDataAsset.h"
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"
#include "GameplayEffectExecutionCalculation.h" // GAS Attribute Capture macros (DECLARE_ATTRIBUTE_CAPTUREDEF, etc.)

#define ATTRIBUTE_CLASS UBaseAttributeSet


#define DECLARE_ST_CAPTUREDEF(AttributeName) \
    DECLARE_ATTRIBUTE_CAPTUREDEF(AttributeName##Source); \
    DECLARE_ATTRIBUTE_CAPTUREDEF(AttributeName##Target);

#define DEFINE_ST_CAPTUREDEF(AttributeName) \
    AttributeName##SourceDef = FGameplayEffectAttributeCaptureDefinition(ATTRIBUTE_CLASS::Get##AttributeName##Attribute(), EGameplayEffectAttributeCaptureSource::Source, false); \
    AttributeName##TargetDef = FGameplayEffectAttributeCaptureDefinition(ATTRIBUTE_CLASS::Get##AttributeName##Attribute(), EGameplayEffectAttributeCaptureSource::Target, false); \
    SourceAttributeMap.Add(ATTRIBUTE_CLASS::Get##AttributeName##Attribute(), AttributeName##SourceDef); \
    TargetAttributeMap.Add(ATTRIBUTE_CLASS::Get##AttributeName##Attribute(), AttributeName##TargetDef);

struct FMMCAttributeStatics
{
    DECLARE_ST_CAPTUREDEF(Level);
    DECLARE_ST_CAPTUREDEF(MaxLevel);
    DECLARE_ST_CAPTUREDEF(XP);
    DECLARE_ST_CAPTUREDEF(MaxXP);
    DECLARE_ST_CAPTUREDEF(Health);
    DECLARE_ST_CAPTUREDEF(MaxHealth);
    DECLARE_ST_CAPTUREDEF(HealthRegen);
    DECLARE_ST_CAPTUREDEF(Stamina);
    DECLARE_ST_CAPTUREDEF(MaxStamina);
    DECLARE_ST_CAPTUREDEF(StaminaRegen);

    DECLARE_ST_CAPTUREDEF(AttackPower);
    DECLARE_ST_CAPTUREDEF(AttackSpeed);
    DECLARE_ST_CAPTUREDEF(AttackRange);
    DECLARE_ST_CAPTUREDEF(SkillAmp);
    DECLARE_ST_CAPTUREDEF(CriticalChance);
    DECLARE_ST_CAPTUREDEF(CriticalDamage);
    DECLARE_ST_CAPTUREDEF(Defense);
    DECLARE_ST_CAPTUREDEF(MoveSpeed);
    DECLARE_ST_CAPTUREDEF(CooldownReduction);
    DECLARE_ST_CAPTUREDEF(Tenacity);

    TMap<FGameplayAttribute, FGameplayEffectAttributeCaptureDefinition> SourceAttributeMap;
    TMap<FGameplayAttribute, FGameplayEffectAttributeCaptureDefinition> TargetAttributeMap;

    FMMCAttributeStatics()
    {
        DEFINE_ST_CAPTUREDEF(Health);
        DEFINE_ST_CAPTUREDEF(MaxHealth);
        DEFINE_ST_CAPTUREDEF(HealthRegen);
        DEFINE_ST_CAPTUREDEF(Stamina);
        DEFINE_ST_CAPTUREDEF(MaxStamina);
        DEFINE_ST_CAPTUREDEF(StaminaRegen);

        DEFINE_ST_CAPTUREDEF(AttackPower);
        DEFINE_ST_CAPTUREDEF(AttackSpeed);
        DEFINE_ST_CAPTUREDEF(AttackRange);
        DEFINE_ST_CAPTUREDEF(SkillAmp);
        DEFINE_ST_CAPTUREDEF(CriticalChance);
        DEFINE_ST_CAPTUREDEF(CriticalDamage);
        DEFINE_ST_CAPTUREDEF(Defense);
        DEFINE_ST_CAPTUREDEF(MoveSpeed);
        DEFINE_ST_CAPTUREDEF(CooldownReduction);
        DEFINE_ST_CAPTUREDEF(Tenacity);
    }
};

static const FMMCAttributeStatics& MMCAttributeStatics()
{
    static FMMCAttributeStatics Statics;
    return Statics;
}

UBaseModMagnitudeCalculation::UBaseModMagnitudeCalculation()
{
    for (auto& Pair : MMCAttributeStatics().SourceAttributeMap)
    {
        RelevantAttributesToCapture.Add(Pair.Value);
    }

    for (auto& Pair : MMCAttributeStatics().TargetAttributeMap)
    {
        RelevantAttributesToCapture.Add(Pair.Value);
    }
}

float UBaseModMagnitudeCalculation::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
    // 1. 소스 오브젝트(SkillDataAsset) 확인
    const USkillEffectDataAsset* SkillDataAsset = Cast<USkillEffectDataAsset>(Spec.GetContext().GetSourceObject());
    if (IsValid(SkillDataAsset) == false)
    {
        UE_LOG(LogTemp, Warning, TEXT("MMC: [%s] SkillDataAsset is INVALID! (SourceObject not set)"), *GetName());
        return 0.f;
    }

    // 2. 인덱스 태그(IndexTag) 및 매그니튜드 확인
    const FGameplayTag IndexTag = SkillDataAsset->GetIndexTag();
    if (IndexTag.IsValid() == false)
    {
        UE_LOG(LogTemp, Warning, TEXT("MMC: [%s] IndexTag is INVALID in DataAsset!"), *GetName());
        return 0.f;
    }

    float IndexMagnitude = Spec.GetSetByCallerMagnitude(IndexTag, false, -1.f);
    if (IndexMagnitude == -1.f)
    {
        UE_LOG(LogTemp, Warning, TEXT("MMC: [%s] IndexTag [%s] not found in Spec!"), *GetName(), *IndexTag.ToString());
        return 0.f;
    }

    int32 DataIndex = FMath::RoundToInt(IndexMagnitude);
    const FSkillEffectContainer& Container = SkillDataAsset->GetData();

    // 3. 인덱스 범위 확인
    if (Container.SkillEffectDefinition.IsValidIndex(DataIndex) == false)
    {
        UE_LOG(LogTemp, Warning, TEXT("MMC: [%s] Invalid DataIndex: %d"), *GetName(), DataIndex);
        return 0.f;
    }

    const FSkillEffectDefinition& MyDef = Container.SkillEffectDefinition[DataIndex];
    const EDecreaseBy DecreaseBy = MyDef.DamageType;
    float AbilityLevel = FMath::Max(1.0f, Spec.GetLevel()); // 레벨이 0일 경우 방지

    // 4. 기반 수치 계산
    float TotalCalculatedValue = ReturnCalculatedValue(Spec, MyDef, AbilityLevel, MMCAttributeStatics().SourceAttributeMap);

    if (TotalCalculatedValue != 0.f)
    {
        float FinalValue = 0;

        switch (DecreaseBy)
        {
        case EDecreaseBy::Noting:
        {
            FinalValue = TotalCalculatedValue;
            break;
        }
        case EDecreaseBy::Defense:
        {
            float Defense = FindValueByAttribute(Spec, UBaseAttributeSet::GetDefenseAttribute(), MMCAttributeStatics().TargetAttributeMap);
            float Mitigation = 100.0f / (100.0f + Defense);
            FinalValue = TotalCalculatedValue * Mitigation;
            break;
        }
        case EDecreaseBy::Tenacity:
        {
            float Tenacity = FindValueByAttribute(Spec, UBaseAttributeSet::GetTenacityAttribute(), MMCAttributeStatics().TargetAttributeMap);
            float ResistanceMultiplier = FMath::Max<float>(1.0f - Tenacity, 0.0f);
            FinalValue = TotalCalculatedValue * ResistanceMultiplier;
            break;
        }
        default:
            FinalValue = TotalCalculatedValue;
            break;
        }

        const EAdjustmentType AdjustmentType = MyDef.Adjustment;
        FinalValue *= (AdjustmentType == EAdjustmentType::Add ? 1.0f : -1.0f);
        
        // UE_LOG(LogTemp, Log, TEXT("MMC: [%s] Calculated Final Magnitude: %f"), *GetName(), FinalValue);
        return FinalValue;
    }

    return 0.f;
}



float UBaseModMagnitudeCalculation::FindValueByAttribute(const FGameplayEffectSpec& Spec, const FGameplayAttribute& Attribute, const TMap<FGameplayAttribute, FGameplayEffectAttributeCaptureDefinition>& TargetMap) const
{
    float FoundValue = 0.f;

    if (const FGameplayEffectAttributeCaptureDefinition* FoundDef = TargetMap.Find(Attribute))
    {
        // MMC에서는 GetCapturedAttributeMagnitude를 사용합니다.
        GetCapturedAttributeMagnitude(*FoundDef, Spec, FAggregatorEvaluateParameters(), FoundValue);
    }

    return FoundValue;
}

float UBaseModMagnitudeCalculation::ReturnCalculatedValue(const FGameplayEffectSpec& Spec, const FSkillEffectDefinition& SkillEffectDefinition, const float Level, const TMap<FGameplayAttribute, FGameplayEffectAttributeCaptureDefinition>& SelectedMap) const
{
    float TotalCalculatedValue = 0.f;

    for (const FSkillAttributeData& AttrData : SkillEffectDefinition.SkillAttributeData)
    {
        const float StatValue = FindValueByAttribute(Spec, AttrData.SourceAttribute, SelectedMap);

        const float Coeff = AttrData.Coefficients.GetValueAtLevel(Level);
        const float BaseVal = AttrData.BasedValues.GetValueAtLevel(Level);

        TotalCalculatedValue += (StatValue * Coeff) + BaseVal;
    }

    return TotalCalculatedValue;
}
