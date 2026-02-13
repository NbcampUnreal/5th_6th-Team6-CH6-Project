// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "TargetActor.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTER_API ATargetActor : public AGameplayAbilityTargetActor
{
	GENERATED_BODY()
public:
	ATargetActor();
	virtual void BeginPlay() override;
	virtual void StartTargeting(UGameplayAbility* Ability) override;
	virtual void ConfirmTargetingAndContinue() override;
	bool TryConfirmMouseTarget();
private:
	
};
