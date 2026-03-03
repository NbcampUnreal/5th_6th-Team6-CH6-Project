// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/SkillDataAsset.h"
#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "GameAbility/SkillBase.h"
#include "SkillConfig/BaseSkillConfig.h"

FGameplayAbilitySpec USkillDataAsset::MakeSpec()
{
	TSubclassOf<USkillBase> AbilityClass = SkillConfig->AbilityClass;
	TSubclassOf<UGameplayAbility> ClassToUse = AbilityClass ? AbilityClass : TSubclassOf<UGameplayAbility>(USkillBase::StaticClass());

	FGameplayAbilitySpec Spec(ClassToUse, 1);

	Spec.SourceObject = this;

	Spec.DynamicAbilityTags.AddTag(SkillConfig->Data.InputKeyTag);

    return Spec;
}

FSkillTooltipData USkillDataAsset::GetSkillTooltipData(float InLevel) const
{
	FSkillTooltipData Result;

	if (!IsValid(SkillConfig))
	{
		return Result;
	}

	const FSkillDefaultData& DefaultData = SkillConfig->Data;
	Result.SkillName = SkillName;
	Result.ShortDescription = ShortDescription;
	Result.DetailedDescription = DetailedDescription;

	TArray<FString> EffectDescriptions;
	for (USkillEffectDataAsset* EffectDataAsset : SkillConfig->GetExcutionEffects())
	{
		if (!IsValid(EffectDataAsset))
		{
			continue;
		}

		const FString EffectDescription = EffectDataAsset->BuildEffectDescription(InLevel).ToString();
		if (!EffectDescription.IsEmpty())
		{
			EffectDescriptions.Add(EffectDescription);
		}
	}

	Result.SkillEffectDescription = FText::FromString(FString::Join(EffectDescriptions, TEXT("\n")));

	if (!Result.SkillEffectDescription.IsEmpty())
	{
		if (!Result.DetailedDescription.IsEmpty())
		{
			Result.DetailedDescription = FText::FromString(FString::Printf(TEXT("%s\n\n%s"), *Result.DetailedDescription.ToString(), *Result.SkillEffectDescription.ToString()));
		}
		else
		{
			Result.DetailedDescription = Result.SkillEffectDescription;
		}
	}

	Result.CooldownSeconds = DefaultData.BaseCoolTime.GetValueAtLevel(InLevel);
	Result.CostDescription = SkillConfig->BuildCostDescription(InLevel);

	return Result;
}