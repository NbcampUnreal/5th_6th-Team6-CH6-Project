// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectComponent/MoveBaseGEC.h"
#include "ConstantForceMoveGEC.generated.h"

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API UConstantForceMoveGECConfig : public UMoveBaseConfig
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "ConstantForce")
	float MoveSpeed = 1500.0f;
};

UCLASS()
class PROJECTER_API UConstantForceMoveGEC : public UMoveBaseGEC
{
	GENERATED_BODY()

public:
	UConstantForceMoveGEC();
	virtual TSubclassOf<UBaseGECConfig> GetRequiredConfigClass() const override;

	virtual float CalculateMoveDuration(const AActor* Instigator, const FVector& Direction, const UMoveBaseConfig* Config) const override;

protected:
	virtual void Execute(AActor* Instigator, const FVector& Direction, const UMoveBaseConfig* Config, const FGameplayEffectSpec& GESpec) const override;
};
