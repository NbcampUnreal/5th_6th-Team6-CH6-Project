// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplayEffectComponent/LaunchProjectile.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "GameFramework/Actor.h"
#include "SkillSystem/GameplayEffectComponent/BaseGECConfig.h"
#include "SkillSystem/Actor/BaseRangeOverlapEffectActor.h"
#include "SkillSystem/GameplyeEffect/SkillEffectDataAsset.h"
#include "SkillSystem/GameAbility/SkillBase.h"

ULaunchProjectile::ULaunchProjectile()
{
	ConfigClass = ULaunchProjectileConfig::StaticClass();
}

TSubclassOf<UBaseGECConfig> ULaunchProjectile::GetRequiredConfigClass() const
{
	return TSubclassOf<ULaunchProjectileConfig>();
}

FText ULaunchProjectileConfig::BuildTooltipDescription(float InLevel) const
{
	TArray<FString> AppliedDescriptions;

	for (const USkillEffectDataAsset* SkillEffectDataAsset : Applied)
	{
		if (!IsValid(SkillEffectDataAsset))
		{
			continue;
		}

		const FString Desc = SkillEffectDataAsset->BuildEffectDescription(InLevel).ToString();
		if (!Desc.IsEmpty())
		{
			AppliedDescriptions.Add(Desc);
		}
	}

	if (AppliedDescriptions.IsEmpty())
	{
		return FText::GetEmpty();
	}

	return FText::FromString(FString::Join(AppliedDescriptions, TEXT("\n")));
}