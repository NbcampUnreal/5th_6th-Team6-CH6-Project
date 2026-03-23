// Fill out your copyright notice in the Description page of Project Settings.

#include "StackRewardGEC.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "SkillSystem/SkillNiagaraSpawnConfig.h"
#include "SkillSystem/GameplyeEffect/SkillEffectDataAsset.h"

UStackRewardGEC::UStackRewardGEC()
{
	ConfigClass = UStackRewardGECConfig::StaticClass();
}

TSubclassOf<UBaseGECConfig> UStackRewardGEC::GetRequiredConfigClass() const
{
	return UStackRewardGECConfig::StaticClass();
}

bool UStackRewardGEC::OnActiveGameplayEffectAdded(FActiveGameplayEffectsContainer &ActiveGEContainer, FActiveGameplayEffect &ActiveGE) const
{
    bool bResult = Super::OnActiveGameplayEffectAdded(ActiveGEContainer, ActiveGE);

    const UStackRewardGECConfig *Config = ResolveTypedConfigFromSpec<UStackRewardGECConfig>(ActiveGE.Spec);

    // 1. Config 유효성 및 보상 목록 개수 체크
    if (!IsValid(Config))
    {        
        return bResult;
    }

    if (Config->Rewards.Num() <= 0)
    {
        return bResult;
    }

    // 2. Target ASC 유효성 체크
    UAbilitySystemComponent *TargetASC = ActiveGEContainer.Owner;
    if (!IsValid(TargetASC))
    {
        return bResult;
    }

    // 람다 바인딩: 스택이 변경될 때마다 호출
    ActiveGE.EventSet.OnStackChanged.AddLambda(
        [this, Config, TargetASC](FActiveGameplayEffectHandle InHandle, int32 NewStack, int32 OldStack)
        { 
			ProcessStackRewards(TargetASC, InHandle, NewStack, Config);
        });

    // 3. 최초 부여 시점(1스택) 체크 로그
    int32 InitialStack = ActiveGE.Spec.GetStackCount();
    ProcessStackRewards(TargetASC, ActiveGE.Handle, InitialStack, Config);

    return bResult;
}

void UStackRewardGEC::ProcessStackRewards(UAbilitySystemComponent* TargetASC, FActiveGameplayEffectHandle InHandle, int32 CurrentStack, const UStackRewardGECConfig* Config) const
{
	for (const FStackRewardInfo& RewardInfo : Config->Rewards)
	{
		if (CurrentStack == RewardInfo.StackCount)
		{
			const FActiveGameplayEffect* Effect = TargetASC->GetActiveGameplayEffect(InHandle);
			if (!Effect) continue;
			UAbilitySystemComponent* SourceASC = Effect->Spec.GetContext().GetInstigatorAbilitySystemComponent();
			if (IsValid(SourceASC))
			{
				if (IsValid(RewardInfo.SkillEffectDataAsset))
				{
					// 데이터 에셋(SkillEffectDataAsset)으로부터 강화 효과 스펙(들) 생성
					TArray<FGameplayEffectSpecHandle> RewardSpecs = RewardInfo.SkillEffectDataAsset->MakeSpecs(SourceASC, const_cast<UGameplayAbility*>(Effect->Spec.GetContext().GetAbility()),  Effect->Spec.GetContext().GetEffectCauser(),  Effect->Spec.GetContext());
					for (const FGameplayEffectSpecHandle& SpecHandle : RewardSpecs)
					{
						if (SpecHandle.IsValid())
						{
							if (RewardInfo.bApplyToTarget)
							{
								SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
							}
							else
							{
								SourceASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
							}
						}
					}
				}

				{
					// --- VFX 발동 로직 ---
					// 1. 시전자(Instigator) 피드백 VFX
					FScopedPredictionWindow ForcedWindow(SourceASC, FPredictionKey(), false);
					if (IsValid(RewardInfo.InstigatorVfxConfig) && RewardInfo.InstigatorVfxConfig->CueTag.IsValid())
					{
						FGameplayCueParameters Params(Effect->Spec);
						Params.SourceObject = RewardInfo.InstigatorVfxConfig;
						SourceASC->ExecuteGameplayCue(RewardInfo.InstigatorVfxConfig->CueTag, Params);
					}
					// 2. 발동 대상(Target) 피드백 VFX
					if (IsValid(RewardInfo.TargetVfxConfig) && RewardInfo.TargetVfxConfig->CueTag.IsValid())
					{
						FGameplayCueParameters Params(Effect->Spec);
						Params.SourceObject = RewardInfo.TargetVfxConfig;
						
						// 대상 액터 본체에 어태치
						if (AActor *TargetAvatar = TargetASC->GetAvatarActor())
						{
							Params.TargetAttachComponent = TargetAvatar->GetRootComponent();
						}
						
						SourceASC->ExecuteGameplayCue(RewardInfo.TargetVfxConfig->CueTag, Params);
					}
				}
			}
			if (RewardInfo.bClearStack)
			{
				TargetASC->RemoveActiveGameplayEffect(InHandle, -1);
			}
		}
	}
}