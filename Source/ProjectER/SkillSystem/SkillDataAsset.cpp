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