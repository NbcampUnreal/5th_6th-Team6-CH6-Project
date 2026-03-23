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
class UAbilitySystemComponent;
class UGameplayAbility;
struct FGameplayEffectContextHandle;
struct FGameplayEffectSpecHandle;
struct FGameplayEffectSpec;

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

	// Config 해석 유틸리티 (파생 클래스 및 헬퍼에서 공용 사용)
	static const UBaseGECConfig* ResolveBaseConfigFromSpec(const FGameplayEffectSpec& GESpec);

	template <typename TConfig>
	static const TConfig* ResolveTypedConfigFromSpec(const FGameplayEffectSpec& GESpec);

	static void GetSkillProcEffects(UAbilitySystemComponent* InstigatorASC, UGameplayAbility* InstigatorSkill,  AActor* InEffectCauser,  const FGameplayEffectContextHandle& CurrentContext,  TArray<FGameplayEffectSpecHandle>& OutSpecs, bool bDefaultConsume = true);
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
const TConfig* UBaseGEC::ResolveTypedConfigFromSpec(const FGameplayEffectSpec& GESpec)
{
	static_assert(TIsDerivedFrom<TConfig, UBaseGECConfig>::Value, "TConfig must derive from UBaseGECConfig");

	const UBaseGECConfig* const BaseConfig = ResolveBaseConfigFromSpec(GESpec);
	if (!IsValid(BaseConfig))
	{
		return nullptr;
	}

	return Cast<TConfig>(BaseConfig);
}
