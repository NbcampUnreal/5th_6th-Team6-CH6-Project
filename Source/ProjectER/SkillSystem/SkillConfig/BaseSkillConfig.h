// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SkillSystem/SkillData.h"
#include "BaseSkillConfig.generated.h"

/**
 * 
 */

class USkillBase;

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API UBaseSkillConfig : public UObject
{
	GENERATED_BODY()
	
public:
	UBaseSkillConfig();

	UPROPERTY(EditDefaultsOnly, Category = "Data")
	FSkillDefaultData SkillData;

protected:
	UPROPERTY(VisibleAnywhere, Category = "AbilityType", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<USkillBase> AbilityClass;

	/*UPROPERTY(EditDefaultsOnly, Category = "Skill|Effect")
	TArray<FSkillEffectData> EffectDatas;*/
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API UMouseTargetSkillConfig : public UBaseSkillConfig
{
	GENERATED_BODY()

public:
	UMouseTargetSkillConfig();
	FORCEINLINE float GetRange() const { return Range; }
	FORCEINLINE const TArray<FSkillEffectData>& GetEffects() const { return EffectsToApply; }
protected:
	UPROPERTY(EditDefaultsOnly, Category = "AbilityType", meta = (AllowPrivateAccess = "true"))
	float Range;

	UPROPERTY(EditDefaultsOnly, Category = "Skill|Effect")
	TArray<FSkillEffectData> EffectsToApply;
};
