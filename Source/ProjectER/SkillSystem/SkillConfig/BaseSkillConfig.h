// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BaseSkillConfig.generated.h"

/**
 * 
 */

class USkillBase;

UCLASS()
class PROJECTER_API UBaseSkillConfig : public UObject
{
	GENERATED_BODY()
	
public:

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Default|Ability")
	TSoftClassPtr<USkillBase> Ability;

};
