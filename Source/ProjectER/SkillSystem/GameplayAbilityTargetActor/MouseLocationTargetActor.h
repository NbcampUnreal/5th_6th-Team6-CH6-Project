// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "MouseLocationTargetActor.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTER_API AMouseLocationTargetActor : public AGameplayAbilityTargetActor
{
	GENERATED_BODY()
	
public:
public:
	virtual void ConfirmTargetingAndContinue() override;
	bool TryConfirmMouseLocation();
	bool SubmitExternalLocation(const FVector& InLocation);
};
