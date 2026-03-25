// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SkillSystem/SkillData.h"
#include "SkillSystem/GameplayEffect/SkillEffectDataAsset.h"
#include "BaseSkillConfig.generated.h"

/**
 * 
 */

class USkillBase;

USTRUCT(BlueprintType)
struct FSkillCostInfo
{
	GENERATED_BODY()

public:
	// 소모할 스탯 (예: Mana, Stamina 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
	FGameplayAttribute Attribute;

	// 소모량 (FScalableFloat를 사용하여 레벨/커브 대응)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
	FScalableFloat CostValue;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API UBaseSkillConfig : public UObject
{
	GENERATED_BODY()
	
public:
	UBaseSkillConfig();

	UPROPERTY(EditDefaultsOnly, Category = "DefaultData")
	FSkillDefaultData Data;

	UPROPERTY(EditDefaultsOnly, Category = "DefaultData|Cost")
	TArray<FSkillCostInfo> SkillCosts;

	UPROPERTY(VisibleAnywhere, Category = "Config", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<USkillBase> AbilityClass;

	FORCEINLINE const TArray<TObjectPtr<USkillEffectDataAsset>>& GetExecutionEffects() const { return ExecutionEffects; }
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TArray<TObjectPtr<USkillEffectDataAsset>> ExecutionEffects;

public:
	UGameplayEffect* CreateCostGameplayEffect(UObject* Outer);

	UFUNCTION(BlueprintPure, Category = "Skill|UI")
	FText BuildCostDescription(float InLevel = 1.0f) const;
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

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API UInstantSkillConfig : public UBaseSkillConfig
{
	GENERATED_BODY()

public:
	UInstantSkillConfig();
protected:
};