// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SkillSystem/SkillData.h"
#include "SkillSystem/GameplyeEffect/SkillEffectDataAsset.h"
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

	UPROPERTY(EditDefaultsOnly, Category = "DefaultData")
	FSkillDefaultData Data;

	UPROPERTY(VisibleAnywhere, Category = "DefaultData", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<USkillBase> AbilityClass;
protected:

};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API UMouseTargetSkillConfig : public UBaseSkillConfig
{
	GENERATED_BODY()

public:
	UMouseTargetSkillConfig();
	FORCEINLINE float GetRange() const { return Range; }
	FORCEINLINE const TArray<TObjectPtr<USkillEffectDataAsset>>& GetEffectDataAssets() const { return EffectsToApply; }
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Config", meta = (AllowPrivateAccess = "true"))
	float Range;

	UPROPERTY(EditDefaultsOnly)
	TArray<TObjectPtr<USkillEffectDataAsset>> EffectsToApply;
};
