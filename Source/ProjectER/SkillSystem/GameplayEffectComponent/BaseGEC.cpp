// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplayEffectComponent/BaseGEC.h"
#include "SkillSystem/GameplayEffectComponent/BaseGECConfig.h"

UBaseGEC::UBaseGEC()
{
	ConfigClass = UBaseGEC::StaticClass();
}

TSubclassOf<UBaseGECConfig> UBaseGEC::GetRequiredConfigClass() const
{
	return UBaseGEC::StaticClass();
}

void UBaseGEC::OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
	Super::OnGameplayEffectExecuted(ActiveGEContainer, GESpec, PredictionKey);
}
