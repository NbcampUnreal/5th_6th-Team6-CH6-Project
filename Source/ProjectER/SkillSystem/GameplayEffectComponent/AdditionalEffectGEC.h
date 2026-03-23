// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectComponent/BaseGECConfig.h"
#include "SkillSystem/GameplayEffectComponent/BaseGEC.h"
#include "AdditionalEffectGEC.generated.h"

class USkillEffectDataAsset;
class USkillNiagaraSpawnConfig;

/**
 * Enhancement effect configuration containing a list of SkillEffectDataAssets
 */
UCLASS()
class PROJECTER_API UAdditionalEffectConfig : public UBaseGECConfig
{
	GENERATED_BODY()

public:
	virtual FText BuildTooltipDescription(float InLevel) const override;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TArray<TObjectPtr<USkillEffectDataAsset>> Bonus;

	/** 효과 적용 후 이 버프를 소모(제거)할지 여부 */
	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	bool bConsumeBuff = true;

	/** 버프가 활성화되어 있는 동안 재생할 나이아가라 효과 */
	UPROPERTY(EditDefaultsOnly, Category = "Niagara")
	TObjectPtr<USkillNiagaraSpawnConfig> ActiveVfxConfig;
};

/**
 * GEC used to identify and store additional effects in a SkillProc buff.
 */
UCLASS()
class PROJECTER_API UAdditionalEffectGEC : public UBaseGEC
{
	GENERATED_BODY()

public:
	UAdditionalEffectGEC();

	virtual TSubclassOf<UBaseGECConfig> GetRequiredConfigClass() const override;

	virtual bool OnActiveGameplayEffectAdded(FActiveGameplayEffectsContainer& ActiveGEContainer, FActiveGameplayEffect& ActiveGE) const override;
};
