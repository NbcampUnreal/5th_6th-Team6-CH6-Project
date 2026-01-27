// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "BaseMMC.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTER_API UBaseMMC : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()
	
public:
	UBaseMMC();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
protected:

private:

public:

protected:

private:
};
