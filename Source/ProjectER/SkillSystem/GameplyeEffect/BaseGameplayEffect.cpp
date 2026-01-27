// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplyeEffect/BaseGameplayEffect.h"
#include "SkillSystem/GameplayModMagnitudeCalculation/BaseMMC.h"

UBaseGameplayEffect::UBaseGameplayEffect()
{
	FCustomCalculationBasedFloat CustomCalculationBasedFloat;
	CustomCalculationBasedFloat.CalculationClassMagnitude = UBaseMMC::StaticClass();

	FGameplayModifierInfo ModifierInfo;
	ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(CustomCalculationBasedFloat);
	Modifiers.Add(ModifierInfo);
}
