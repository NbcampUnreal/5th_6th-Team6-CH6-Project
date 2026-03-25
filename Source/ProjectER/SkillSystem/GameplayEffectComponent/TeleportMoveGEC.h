// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectComponent/MoveBaseGEC.h"
#include "TeleportMoveGEC.generated.h"

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API UTeleportMoveGECConfig : public UMoveBaseConfig
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Teleport")
	bool bSweep = true;
};

UCLASS()
class PROJECTER_API UTeleportMoveGEC : public UMoveBaseGEC
{
	GENERATED_BODY()

public:
	UTeleportMoveGEC();
	virtual TSubclassOf<UBaseGECConfig> GetRequiredConfigClass() const override;

	virtual float CalculateMoveDuration(const AActor* Instigator, const FVector& Direction, const UMoveBaseConfig* Config) const override;

protected:
	virtual void Execute(AActor* Instigator, const FVector& Direction, const UMoveBaseConfig* Config, const FGameplayEffectSpec& GESpec) const override;

private:
	FVector CalculateDestination(const AActor* Instigator, const FVector& Direction, const UTeleportMoveGECConfig* Config) const;
	void UpdateLevelTracker(AActor* Actor) const;
};
