// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplayEffectExecutionCalculation/BaseExecutionCalculation.h"
#include "SkillSystem/SkillDataAsset.h"
#include "SkillSystem/GameplyeEffect/SkillEffectDataAsset.h"
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"

// 캡처할 속성 정의 (헬퍼 구조체)
struct FSKillDamageStatics
{
    // 캡처할 속성 정의 (매크로 활용 가능)
    DECLARE_ATTRIBUTE_CAPTUREDEF(Defense);
    DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingDamage);

    FSKillDamageStatics()
    {
        // Target의 방어력을 가져옴 (Snapshot: false -> 적용 시점의 실시간 값 사용)
        DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, Defense, Target, false);
    }
};

static const FSKillDamageStatics& SKillDamageStatics()
{
    static FSKillDamageStatics DStatics;
    return DStatics;
}

UBaseExecutionCalculation::UBaseExecutionCalculation()
{
    RelevantAttributesToCapture.Add(SKillDamageStatics().DefenseDef);
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
        float AbilityLevel = Spec.GetLevel();
        
        float TotalCalculatedValue = ReturnCalculatedValue(ExecutionParams, MyDef, AbilityLevel);

        if (TotalCalculatedValue != 0.f && Container.TargetAttribute.IsValid())
        {
            OutExecutionOutput.AddOutputModifier(
                FGameplayModifierEvaluatedData(Container.TargetAttribute, EGameplayModOp::Additive, TotalCalculatedValue)
            );
        }
    }
    else {
        UE_LOG(LogTemp, Warning, TEXT("[%s] Invalid DataIndex: %d in DataAsset: %s"), *GetName(), DataIndex, *SkillDataAsset->GetName());
    }
}

float UBaseExecutionCalculation::ReturnCalculatedValue(const FGameplayEffectCustomExecutionParameters& ExecutionParams, const FSkillEffectDefinition& SkillEffectDefinition, const float Level) const
{
    float TotalCalculatedValue = 0.f;

    float SourceStatValue; float Coeff; float BaseVal;
    for (const FSkillAttributeData& AttrData : SkillEffectDefinition.SkillAttributeData)
    {
        SourceStatValue = 0.f;
        FGameplayEffectAttributeCaptureDefinition CaptureDef(AttrData.SourceAttribute, EGameplayEffectAttributeCaptureSource::Source, false);

        if (ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(CaptureDef, FAggregatorEvaluateParameters(), SourceStatValue))
        {

        }
        else {
            UE_LOG(LogTemp, Warning, TEXT("123"));
        }

        Coeff = AttrData.Coefficients.GetValueAtLevel(Level);
        BaseVal = AttrData.BasedValues.GetValueAtLevel(Level);

        TotalCalculatedValue += (SourceStatValue * Coeff) + BaseVal;
    }

    return TotalCalculatedValue;
}
