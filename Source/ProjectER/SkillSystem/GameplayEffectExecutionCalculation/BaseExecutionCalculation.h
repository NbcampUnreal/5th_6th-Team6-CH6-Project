// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "BaseExecutionCalculation.generated.h"

/**
 * 
 */
struct FSkillEffectDefinition;
UCLASS()
class PROJECTER_API UBaseExecutionCalculation : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
	

public:
	UBaseExecutionCalculation();
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

protected:
	float ReturnCalculatedValue(const FGameplayEffectCustomExecutionParameters& ExecutionParams, const FSkillEffectDefinition& SkillEffectDefinition, const float Level) const;
private:
};
