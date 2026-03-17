// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectComponent.h"
#include "SkillSystem/GameplayEffectComponent/BaseGECConfig.h"
#include "BaseGEC.generated.h"

/**
 *
 */

class UBaseGECConfig;

UCLASS()
class PROJECTER_API UBaseGEC : public UGameplayEffectComponent
{
	GENERATED_BODY()

public:
	UBaseGEC();

	FORCEINLINE UBaseGECConfig* GetConfig() const { return CachedConfig; }

	virtual TSubclassOf<UBaseGECConfig> GetRequiredConfigClass() const;

protected:
	virtual void OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const override;

	// Config 해석 유틸리티 (파생 클래스에서 공용 사용)
	const UBaseGECConfig* ResolveBaseConfigFromSpec(const FGameplayEffectSpec& GESpec) const;

	template <typename TConfig>
	const TConfig* ResolveTypedConfigFromSpec(const FGameplayEffectSpec& GESpec) const;

private:

public:

protected:
	UPROPERTY()
	TObjectPtr<UBaseGECConfig> CachedConfig;

	UPROPERTY()
	TSubclassOf<UBaseGECConfig> ConfigClass;
private:

};

template <typename TConfig>
const TConfig* UBaseGEC::ResolveTypedConfigFromSpec(const FGameplayEffectSpec& GESpec) const
{
	static_assert(TIsDerivedFrom<TConfig, UBaseGECConfig>::Value, "TConfig must derive from UBaseGECConfig");

	const UBaseGECConfig* const BaseConfig = ResolveBaseConfigFromSpec(GESpec);
	if (!IsValid(BaseConfig))
	{
		return nullptr;
	}

	return Cast<TConfig>(BaseConfig);
}
