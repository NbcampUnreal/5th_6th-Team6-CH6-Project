// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/Actor/BaseRangeOverlapEffectActor.h"
#include "CapsuleRangeOverlapEffectActor.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTER_API ACapsuleRangeOverlapEffectActor : public ABaseRangeOverlapEffectActor
{
	GENERATED_BODY()
public:
	ACapsuleRangeOverlapEffectActor();

protected:
	virtual void ApplyCollisionSize(float InCollisionSize) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Collision")
	TObjectPtr<UCapsuleComponent> CapsuleComponent;
};
