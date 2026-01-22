// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "SkillDataAsset.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum class ESkillActivationType : uint8 {
    Instant    UMETA(DisplayName = "Instant"),
    Targeted   UMETA(DisplayName = "Targeted"),
    PointClick UMETA(DisplayName = "PointClick"),
    ClickAndDrag       UMETA(DisplayName = "ClickAndDrag"),
    Holding    UMETA(DisplayName = "Holding")
};

UENUM(BlueprintType)
enum class ESkillFormType : uint8 {
    Fixed      UMETA(DisplayName = "고정 위치(소환)"),
    Projectile UMETA(DisplayName = "투사체(직선)"),
    Homing     UMETA(DisplayName = "추적(호밍)"),
    Attached   UMETA(DisplayName = "부착형")
};

USTRUCT(BlueprintType)
struct FSkillModifierInfo {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    FGameplayTag StatTag; // 참조할 스탯 (예: Stat.AttackPower, Stat.AbilityPower)

    UPROPERTY(EditAnywhere)
    float Coefficient;    // 계수 (예: 0.6, 1.2)

    UPROPERTY(EditAnywhere)
    float BaseDamage;     // 스킬 기본 데미지
};

UCLASS()
class PROJECTER_API USkillDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:



    UPROPERTY(EditDefaultsOnly, Category = "Skill|Damage")
    TArray<FSkillModifierInfo> DamageModifiers;
};
