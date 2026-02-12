// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplayEffectExecutionCalculation/BaseExecutionCalculation.h"
#include "SkillSystem/SkillDataAsset.h"
#include "SkillSystem/GameplyeEffect/SkillEffectDataAsset.h"
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"

#define ATTRIBUTE_CLASS UBaseAttributeSet

#define DECLARE_ST_CAPTUREDEF(AttributeName) \
    DECLARE_ATTRIBUTE_CAPTUREDEF(AttributeName##Source); \
    DECLARE_ATTRIBUTE_CAPTUREDEF(AttributeName##Target);

#define DEFINE_ST_CAPTUREDEF(AttributeName) \
    AttributeName##SourceDef = FGameplayEffectAttributeCaptureDefinition(ATTRIBUTE_CLASS::Get##AttributeName##Attribute(), EGameplayEffectAttributeCaptureSource::Source, false); \
    AttributeName##TargetDef = FGameplayEffectAttributeCaptureDefinition(ATTRIBUTE_CLASS::Get##AttributeName##Attribute(), EGameplayEffectAttributeCaptureSource::Target, false); \
    SourceAttributeMap.Add(ATTRIBUTE_CLASS::Get##AttributeName##Attribute(), AttributeName##SourceDef); \
    TargetAttributeMap.Add(ATTRIBUTE_CLASS::Get##AttributeName##Attribute(), AttributeName##TargetDef);

struct FAttributeStatics
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

    DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingDamage); // 메타 속성 (기존 방식)

    TMap<FGameplayAttribute, FGameplayEffectAttributeCaptureDefinition> SourceAttributeMap;
    TMap<FGameplayAttribute, FGameplayEffectAttributeCaptureDefinition> TargetAttributeMap;

    FAttributeStatics()
    {
        // --- 2. 정의 및 맵 등록 (헬퍼 하나로 4가지 작업 동시 수행) ---
        //DEFINE_ST_CAPTUREDEF(Level);
        //DEFINE_ST_CAPTUREDEF(MaxLevel);
        //DEFINE_ST_CAPTUREDEF(XP);
        //DEFINE_ST_CAPTUREDEF(MaxXP);
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

        // IncomingDamage는 Target 전용이므로 별도 등록
        DEFINE_ATTRIBUTE_CAPTUREDEF(ATTRIBUTE_CLASS, IncomingDamage, Target, false);
        TargetAttributeMap.Add(ATTRIBUTE_CLASS::GetIncomingDamageAttribute(), IncomingDamageDef);
    }
};

static const FAttributeStatics& AttributeStatics()
{
    static FAttributeStatics Statics;
    return Statics;
}

UBaseExecutionCalculation::UBaseExecutionCalculation()
{
    // 생성자에서 맵에 등록된 모든 Def를 엔진에 알립니다.
    // 이 작업이 있어야 AttemptCalculate... 함수가 0이 아닌 실제 값을 반환합니다.
    for (auto& Pair : AttributeStatics().SourceAttributeMap)
    {
        RelevantAttributesToCapture.Add(Pair.Value);
    }

    for (auto& Pair : AttributeStatics().TargetAttributeMap)
    {
        RelevantAttributesToCapture.Add(Pair.Value);
    }
}

void UBaseExecutionCalculation::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	Super::Execute_Implementation(ExecutionParams, OutExecutionOutput);

    const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
    const USkillEffectDataAsset* SkillDataAsset = Cast<USkillEffectDataAsset>(Spec.GetContext().GetSourceObject());

    if (IsValid(SkillDataAsset) == false) return;    
    //SetByCaller에서 인덱스 추출
    const FGameplayTag IndexTag = SkillDataAsset->GetIndexTag();
    // 값이 없을 경우를 대비해 기본값 -1
    int32 DataIndex = FMath::RoundToInt(Spec.GetSetByCallerMagnitude(IndexTag, false, -1.f));

    FGameplayAttribute TargetAttr = SkillDataAsset->GetTargetAttribute();
    const FSkillEffectContainer& Container = SkillDataAsset->GetData();

    if (Container.TargetAttribute.IsValid() == false)
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] TargetAttribute is INVALID in DataAsset: %s"), *GetName(), *SkillDataAsset->GetName());
        return;
    }

    // 2. 인덱스 유효성 검사 후 해당 데이터 추출
    if (Container.SkillEffectDefinition.IsValidIndex(DataIndex))
    {
        const FSkillEffectDefinition& MyDef = Container.SkillEffectDefinition[DataIndex];
        const EDecreaseBy DecreaseBy = MyDef.DamageType;
        float AbilityLevel = Spec.GetLevel();
        
        float TotalCalculatedValue = ReturnCalculatedValue(ExecutionParams, MyDef, AbilityLevel, AttributeStatics().SourceAttributeMap);

        if (TotalCalculatedValue != 0.f && Container.TargetAttribute.IsValid())
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
                float Defense = FindValueByAttribute(ExecutionParams, UBaseAttributeSet::GetDefenseAttribute(), AttributeStatics().TargetAttributeMap);
                float Mitigation = 100.0f / (100.0f + Defense);
                FinalValue = TotalCalculatedValue * Mitigation;
				break;
            }
            case EDecreaseBy::Tenacity:
            {
                float Tenacity = FindValueByAttribute(ExecutionParams, UBaseAttributeSet::GetTenacityAttribute(), AttributeStatics().TargetAttributeMap);
                float ResistanceMultiplier = FMath::Max<float>(1.0f - Tenacity, 0.0f);
                FinalValue = TotalCalculatedValue * ResistanceMultiplier;
				break;
            }
            default:
                break;
            }

            //값을 더할건지 뺄건지 결정
            const EAdjustmentType AdjustmentType = MyDef.Adjustment;
            FinalValue *= AdjustmentType == EAdjustmentType::Add ? 1 : -1;
            OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(Container.TargetAttribute, EGameplayModOp::Additive, FinalValue));
        }
    }
    else {
        UE_LOG(LogTemp, Warning, TEXT("[%s] Invalid DataIndex: %d in DataAsset: %s"), *GetName(), DataIndex, *SkillDataAsset->GetName());
    }
}

float UBaseExecutionCalculation::FindValueByAttribute(const FGameplayEffectCustomExecutionParameters& ExecutionParams, const FGameplayAttribute& Attribute, const TMap<FGameplayAttribute, FGameplayEffectAttributeCaptureDefinition>& TargetMap) const
{
    float FoundValue = 0.f;

    // 인자로 넘어온 맵에서 정의를 찾습니다.
    if (const FGameplayEffectAttributeCaptureDefinition* FoundDef = TargetMap.Find(Attribute))
    {
        ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(*FoundDef, FAggregatorEvaluateParameters(), FoundValue);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ExecutionCalculation: 선택된 맵에 [%s] 속성이 등록되지 않았습니다!"), *Attribute.GetName());
    }

    return FoundValue;
}

float UBaseExecutionCalculation::ReturnCalculatedValue(const FGameplayEffectCustomExecutionParameters& ExecutionParams, const FSkillEffectDefinition& SkillEffectDefinition, const float Level, const TMap<FGameplayAttribute, FGameplayEffectAttributeCaptureDefinition>& SelectedMap) const
{
    float TotalCalculatedValue = 0.f;

    for (const FSkillAttributeData& AttrData : SkillEffectDefinition.SkillAttributeData)
    {
        // 넘겨받은 맵을 사용하여 스탯 값 추출
        const float StatValue = FindValueByAttribute(ExecutionParams, AttrData.SourceAttribute, SelectedMap);

        const float Coeff = AttrData.Coefficients.GetValueAtLevel(Level);
        const float BaseVal = AttrData.BasedValues.GetValueAtLevel(Level);

        TotalCalculatedValue += (StatValue * Coeff) + BaseVal;
    }

    return TotalCalculatedValue;
}
