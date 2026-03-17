// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplayEffectComponent/BaseGEC.h"
#include "SkillSystem/GameplayEffectComponent/BaseGECConfig.h"
#include "SkillSystem/GameplyeEffect/SkillEffectDataAsset.h"
#include "GameplayEffect.h"

UBaseGEC::UBaseGEC()
{
	ConfigClass = UBaseGECConfig::StaticClass();
}

TSubclassOf<UBaseGECConfig> UBaseGEC::GetRequiredConfigClass() const
{
	return UBaseGECConfig::StaticClass();
}

void UBaseGEC::OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
	Super::OnGameplayEffectExecuted(ActiveGEContainer, GESpec, PredictionKey);
}

const UBaseGECConfig* UBaseGEC::ResolveBaseConfigFromSpec(const FGameplayEffectSpec& GESpec) const
{
	const USkillEffectDataAsset* const SkillDataAsset = Cast<USkillEffectDataAsset>(GESpec.GetEffectContext().GetSourceObject());
	if (!IsValid(SkillDataAsset))
	{
		return nullptr;
	}

	const FGameplayTag IndexTag = SkillDataAsset->GetIndexTag();
	const int32 DataIndex = FMath::RoundToInt(GESpec.GetSetByCallerMagnitude(IndexTag, false, -1.0f));
	const FSkillEffectContainer& SkillContainer = SkillDataAsset->GetData();
	if (!SkillContainer.SkillEffectDefinition.IsValidIndex(DataIndex))
	{
		return nullptr;
	}

	return SkillContainer.SkillEffectDefinition[DataIndex].Config;
}
