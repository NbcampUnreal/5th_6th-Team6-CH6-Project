// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/Actor/BaseRangeOverlapEffectActor.h"
#include "SphereOverlapEffectActor.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTER_API ASphereOverlapEffectActor : public ABaseRangeOverlapEffectActor
{
	GENERATED_BODY()
public:
	ASphereOverlapEffectActor();

protected:
	virtual void ApplyCollisionSize(float InCollisionSize) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Collision")
	TObjectPtr<USphereComponent> SphereComponent;
};
