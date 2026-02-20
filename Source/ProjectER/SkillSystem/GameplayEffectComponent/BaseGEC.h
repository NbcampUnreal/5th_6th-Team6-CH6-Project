// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectComponent.h"
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
private:

public:

protected:
	UPROPERTY()
	TObjectPtr<UBaseGECConfig> CachedConfig;

	UPROPERTY()
	TSubclassOf<UBaseGECConfig> ConfigClass;
private:
	
};
