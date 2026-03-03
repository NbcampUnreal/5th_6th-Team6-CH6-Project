// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SkillData.h"
#include "SkillDataAsset.generated.h"

/**
 * 
 */

//UENUM(BlueprintType)
//enum class ESkillFormType : uint8 {
//    Fixed      UMETA(DisplayName = "고정 위치(소환)"),
//    Projectile UMETA(DisplayName = "투사체(직선)"),
//    Homing     UMETA(DisplayName = "추적(호밍)"),
//    Attached   UMETA(DisplayName = "부착형")
//};

USTRUCT(BlueprintType)
struct FSkillTooltipData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Skill")
    FText SkillName;

    UPROPERTY(BlueprintReadOnly, Category = "Skill")
    FText ShortDescription;

    UPROPERTY(BlueprintReadOnly, Category = "Skill")
    FText DetailedDescription;

    UPROPERTY(BlueprintReadOnly, Category = "Skill")
    FText SkillEffectDescription;

    /*UPROPERTY(BlueprintReadOnly, Category = "Skill")
    FGameplayTag InputKeyTag;*/

    UPROPERTY(BlueprintReadOnly, Category = "Skill")
    float CooldownSeconds = 0.0f;
};

class UAbilitySystemComponent;
class USkillBase;
class UBaseSkillConfig;

UCLASS()
class PROJECTER_API USkillDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
    FGameplayAbilitySpec MakeSpec();

    UFUNCTION(BlueprintPure, Category = "Skill|UI")
    FSkillTooltipData GetSkillTooltipData(float InLevel = 1.0f) const;

private:
    
public:
    UPROPERTY(EditDefaultsOnly, Instanced)
    TObjectPtr<UBaseSkillConfig> SkillConfig;

protected:
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    FText SkillName;

    UPROPERTY(EditDefaultsOnly, Category = "UI", meta = (MultiLine = "true"))
    FText ShortDescription;

    UPROPERTY(EditDefaultsOnly, Category = "UI", meta = (MultiLine = "true"))
    FText DetailedDescription;
};
