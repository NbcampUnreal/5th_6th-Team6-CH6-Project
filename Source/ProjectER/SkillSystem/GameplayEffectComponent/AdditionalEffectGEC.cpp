#include "SkillSystem/GameplayEffectComponent/AdditionalEffectGEC.h"
#include "SkillSystem/GameplyeEffect/SkillEffectDataAsset.h"
#include "SkillSystem/SkillNiagaraSpawnConfig.h"
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
	if (IsValid(Config) && IsValid(Config->ActiveVfxConfig) && Config->ActiveVfxConfig->CueTag.IsValid())
	{
		UAbilitySystemComponent* TargetASC = ActiveGEContainer.Owner;
		if (IsValid(TargetASC))
		{
			FGameplayCueParameters Params(ActiveGE.Spec);
			Params.SourceObject = Config->ActiveVfxConfig;
			Params.Instigator = ActiveGE.Spec.GetContext().GetInstigator();
			Params.EffectCauser = ActiveGE.Spec.GetContext().GetEffectCauser();

			// 1. 버프 지속 동안 나이아가라 루프를 위해 AddGameplayCue 사용
			{
				FScopedPredictionWindow ForcedWindow(TargetASC, FPredictionKey(), false);
				TargetASC->AddGameplayCue(Config->ActiveVfxConfig->CueTag, Params);
			}

			// 2. 버프가 제거될 때(만료 또는 소모) 자동으로 이펙트도 제거되도록 예약
			FGameplayTag CueTag = Config->ActiveVfxConfig->CueTag;
			ActiveGE.EventSet.OnEffectRemoved.AddLambda([TargetASC, CueTag, Params](const FGameplayEffectRemovalInfo& RemovalInfo)
			{
				if (IsValid(TargetASC))
				{
					FScopedPredictionWindow ForcedWindow(TargetASC, FPredictionKey(), false);
					TargetASC->RemoveGameplayCue(CueTag);
				}
			});
		}
	}

	return bResult;
}
