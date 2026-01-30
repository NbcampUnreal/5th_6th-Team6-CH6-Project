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

class UAbilitySystemComponent;
class USkillBase;
class UBaseSkillConfig;

UCLASS()
class PROJECTER_API USkillDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
    FGameplayAbilitySpec MakeSpec();

private:
    
public:
    UPROPERTY(EditDefaultsOnly, Instanced)
    TObjectPtr<UBaseSkillConfig> SkillConfig;
};
