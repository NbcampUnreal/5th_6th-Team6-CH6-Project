#include "SkillSystem/GameplayEffectComponent/AdditionalEffectGEC.h"
#include "SkillSystem/GameplayEffect/SkillEffectDataAsset.h"
#include "SkillSystem/SkillNiagaraSpawnConfig.h"
#include "SkillSystem/SkillSoundSpawnConfig.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

//////////////////////////////////////////////////////////////////////////
// UAdditionalEffectConfig

FText UAdditionalEffectConfig::BuildTooltipDescription(float InLevel) const
{
	TArray<FString> Descriptions;
	for (const auto& Effect : Bonus)
	{
		if (IsValid(Effect))
		{
			Descriptions.Add(Effect->BuildEffectDescription(InLevel).ToString());
		}
	}

	return FText::FromString(FString::Join(Descriptions, TEXT("\n")));
}

//////////////////////////////////////////////////////////////////////////
// UAdditionalEffectGEC

UAdditionalEffectGEC::UAdditionalEffectGEC()
{
	ConfigClass = UAdditionalEffectConfig::StaticClass();
}

TSubclassOf<UBaseGECConfig> UAdditionalEffectGEC::GetRequiredConfigClass() const
{
	return UAdditionalEffectConfig::StaticClass();
}

bool UAdditionalEffectGEC::OnActiveGameplayEffectAdded(FActiveGameplayEffectsContainer& ActiveGEContainer, FActiveGameplayEffect& ActiveGE) const
{
	bool bResult = Super::OnActiveGameplayEffectAdded(ActiveGEContainer, ActiveGE);

	// 0. 다이나믹 에셋 태그 설정 (보너스 효과 식별용)
	static const FGameplayTag SkillProcTag = FGameplayTag::RequestGameplayTag(FName("Skill.Data.Augments"));
	ActiveGE.Spec.AddDynamicAssetTag(SkillProcTag);

	const UAdditionalEffectConfig* const Config = ResolveTypedConfigFromSpec<UAdditionalEffectConfig>(ActiveGE.Spec);
	if (IsValid(Config))
	{
		UAbilitySystemComponent* TargetASC = ActiveGEContainer.Owner;
		if (IsValid(TargetASC))
		{
			// 1. Niagara VFX 처리
			if (IsValid(Config->ActiveVfxConfig.Get()) && Config->ActiveVfxConfig->CueTag.IsValid())
			{
				FGameplayCueParameters Params(ActiveGE.Spec);
				Params.SourceObject = Config->ActiveVfxConfig.Get();
				Params.Instigator = ActiveGE.Spec.GetContext().GetInstigator();
				Params.EffectCauser = ActiveGE.Spec.GetContext().GetEffectCauser();

				{
					FScopedPredictionWindow ForcedWindow(TargetASC, FPredictionKey(), false);
					TargetASC->AddGameplayCue(Config->ActiveVfxConfig->CueTag, Params);
				}

				FGameplayTag CueTag = Config->ActiveVfxConfig->CueTag;
				ActiveGE.EventSet.OnEffectRemoved.AddLambda([TargetASC, CueTag](const FGameplayEffectRemovalInfo& RemovalInfo)
				{
					if (IsValid(TargetASC))
					{
						FScopedPredictionWindow ForcedWindow(TargetASC, FPredictionKey(), false);
						TargetASC->RemoveGameplayCue(CueTag);
					}
				});
			}

			// 2. Sound 처리
			if (IsValid(Config->ActiveSoundConfig.Get()) && Config->ActiveSoundConfig->CueTag.IsValid())
			{
				FGameplayCueParameters Params(ActiveGE.Spec);
				Params.SourceObject = Config->ActiveSoundConfig.Get();
				Params.Instigator = ActiveGE.Spec.GetContext().GetInstigator();
				Params.EffectCauser = ActiveGE.Spec.GetContext().GetEffectCauser();

				{
					FScopedPredictionWindow ForcedWindow(TargetASC, FPredictionKey(), false);
					TargetASC->AddGameplayCue(Config->ActiveSoundConfig->CueTag, Params);
				}

				FGameplayTag CueTag = Config->ActiveSoundConfig->CueTag;
				ActiveGE.EventSet.OnEffectRemoved.AddLambda([TargetASC, CueTag](const FGameplayEffectRemovalInfo& RemovalInfo)
				{
					if (IsValid(TargetASC))
					{
						FScopedPredictionWindow ForcedWindow(TargetASC, FPredictionKey(), false);
						TargetASC->RemoveGameplayCue(CueTag);
					}
				});
			}
		}
	}

	return bResult;
}
