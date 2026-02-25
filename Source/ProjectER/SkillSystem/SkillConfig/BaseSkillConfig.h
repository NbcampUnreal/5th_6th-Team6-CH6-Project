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

	FORCEINLINE const TArray<TObjectPtr<USkillEffectDataAsset>>& GetExcutionEffects() const { return ExcutionEffects; }
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TArray<TObjectPtr<USkillEffectDataAsset>> ExcutionEffects;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API UMouseTargetSkillConfig : public UBaseSkillConfig
{
	GENERATED_BODY()

public:
	UMouseTargetSkillConfig();
	FORCEINLINE float GetRange() const { return Range; }
	FORCEINLINE const TArray<TObjectPtr<USkillEffectDataAsset>>& GetEffectsToApply() const { return EffectsToApply; }
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Config", meta = (AllowPrivateAccess = "true"))
	float Range;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TArray<TObjectPtr<USkillEffectDataAsset>> EffectsToApply;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API UMouseClickSkillConfig : public UBaseSkillConfig
{
	GENERATED_BODY()

public:
	UMouseClickSkillConfig();
	FORCEINLINE float GetRange() const { return Range; }
	//FORCEINLINE const TArray<TObjectPtr<USkillEffectDataAsset>>& GetEffectsToApply() const { return EffectsToApply; }
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Config", meta = (AllowPrivateAccess = "true"))
	float Range;

	//UPROPERTY(EditDefaultsOnly, Category = "Config")
	//TArray<TObjectPtr<USkillEffectDataAsset>> EffectsToApply;
};

