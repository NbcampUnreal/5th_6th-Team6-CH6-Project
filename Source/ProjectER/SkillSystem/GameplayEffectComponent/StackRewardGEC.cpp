// Fill out your copyright notice in the Description page of Project Settings.

#include "StackRewardGEC.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

UStackRewardGEC::UStackRewardGEC()
{
	ConfigClass = UStackRewardGECConfig::StaticClass();
}

TSubclassOf<UBaseGECConfig> UStackRewardGEC::GetRequiredConfigClass() const
{
	return UStackRewardGECConfig::StaticClass();
}

bool UStackRewardGEC::OnActiveGameplayEffectAdded(FActiveGameplayEffectsContainer& ActiveGEContainer, FActiveGameplayEffect& ActiveGE) const
{
	bool bResult = Super::OnActiveGameplayEffectAdded(ActiveGEContainer, ActiveGE);

	// 현재 클래스의 구조에 맞게 Config를 가져옵니다 (UBaseGEC의 ResolveTypedConfigFromSpec 활용)
	const UStackRewardGECConfig* Config = ResolveTypedConfigFromSpec<UStackRewardGECConfig>(ActiveGE.Spec);

	if (!IsValid(Config) || Config->Rewards.Num() <= 0)
	{
		return bResult;
	}

	UAbilitySystemComponent* TargetASC = ActiveGEContainer.Owner;
	if (!IsValid(TargetASC))
	{
		return bResult;
	}

	// 람다 바인딩: 스택이 변경될 때마다(2스택, 3스택 ... 5스택) 호출됩니다.
	ActiveGE.EventSet.OnStackChanged.AddLambda([this, Config, TargetASC](FActiveGameplayEffectHandle InHandle, int32 NewStack, int32 OldStack)
	{
		if (TargetASC)
		{
			ProcessStackRewards(TargetASC, InHandle, NewStack, Config);
		}
	});

	// 혹시 최초 부여 시점(1스택)부터 보상 스택인 경우 대비
	ProcessStackRewards(TargetASC, ActiveGE.Handle, ActiveGE.Spec.GetStackCount(), Config);

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
			if (IsValid(SourceASC) && IsValid(RewardInfo.RewardGE))
			{
				const UGameplayEffect* RewardGE = RewardInfo.RewardGE->GetDefaultObject<UGameplayEffect>();
				FGameplayEffectSpec* RewardSpec = new FGameplayEffectSpec(RewardGE, Effect->Spec.GetContext(), Effect->Spec.GetLevel());

				if (RewardInfo.bApplyToTarget)
				{
					TargetASC->ApplyGameplayEffectSpecToSelf(*RewardSpec);
				}
				else
				{
					SourceASC->ApplyGameplayEffectSpecToSelf(*RewardSpec);
				}
			}

			if (RewardInfo.bClearStack)
			{
				TargetASC->RemoveActiveGameplayEffect(InHandle, -1);
			}
		}
	}
}
