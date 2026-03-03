// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/SkillConfig/BaseSkillConfig.h"
#include "Skillsystem/GameAbility/SkillBase.h"
#include "SkillSystem/GameAbility/MouseTargetSkill.h"
#include "SkillSystem/GameAbility/MouseClickSkill.h"

UBaseSkillConfig::UBaseSkillConfig()
{
	AbilityClass = USkillBase::StaticClass();
}

UGameplayEffect* UBaseSkillConfig::CreateCostGameplayEffect(UObject* Outer)
{
    if (SkillCosts.Num() <= 0) {
        //UE_LOG(LogTemp, Warning, TEXT("SkillCosts.Num() <= 0 true"));
        return nullptr;
    }
    else {
        UE_LOG(LogTemp, Warning, TEXT("SkillCosts.Num() <= 0 false"));
    }

    // GE 인스턴스 생성 (Outer를 TransientPackage로 설정하여 관리)
    UGameplayEffect* NewCostGE = NewObject<UGameplayEffect>(Outer);
    NewCostGE->DurationPolicy = EGameplayEffectDurationType::Instant;

    for (const FSkillCostInfo& CostInfo : SkillCosts)
    {
        if (!CostInfo.Attribute.IsValid()) continue;

        FGameplayModifierInfo ModInfo;
        ModInfo.Attribute = CostInfo.Attribute;
        ModInfo.ModifierOp = EGameplayModOp::Additive;
        ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(CostInfo.CostValue);
        NewCostGE->Modifiers.Add(ModInfo);
    }

    return NewCostGE;
}

FText UBaseSkillConfig::BuildCostDescription(float InLevel) const
{
    TArray<FString> CostTerms;

    for (const FSkillCostInfo& SkillCost : SkillCosts)
    {
        if (SkillCost.Attribute.IsValid())
        {
            const float CostValue = SkillCost.CostValue.GetValueAtLevel(InLevel);
            FString Term = FString::Printf(TEXT("%s %.0f"), *SkillCost.Attribute.GetName(), CostValue);
            CostTerms.Add(Term);
        }
    }

    if (CostTerms.Num() > 0)
    {
        FString Combined = TEXT("소모: ") + FString::Join(CostTerms, TEXT(", "));
        return FText::FromString(Combined);
    }

    return FText::GetEmpty();
}


UMouseTargetSkillConfig::UMouseTargetSkillConfig()
{
	AbilityClass = UMouseTargetSkill::StaticClass();
	Data.SkillActivationType = ESkillActivationType::Targeted;
}

UMouseClickSkillConfig::UMouseClickSkillConfig()
{
	AbilityClass = UMouseClickSkill::StaticClass();
	Data.SkillActivationType = ESkillActivationType::PointClick;
}
