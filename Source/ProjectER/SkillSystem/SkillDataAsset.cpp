// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/SkillDataAsset.h"
#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "GameAbility/SkillBase.h"

FGameplayAbilitySpec USkillDataAsset::MakeSpec(UAbilitySystemComponent* AbilitySystemComponent, float Level)
{
	TSubclassOf<UGameplayAbility> ClassToUse = AbilityClass ? TSubclassOf<UGameplayAbility>(AbilityClass) : TSubclassOf<UGameplayAbility>(USkillBase::StaticClass());

	FGameplayAbilitySpec Spec(ClassToUse, 1);

	Spec.SourceObject = this;

	Spec.DynamicAbilityTags.AddTag(InputKeyTag);

    return Spec;
}