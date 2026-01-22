// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GameplayEffect.h"
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

//UENUM(BlueprintType)
//enum class ESkillFormType : uint8 {
//    Fixed      UMETA(DisplayName = "고정 위치(소환)"),
//    Projectile UMETA(DisplayName = "투사체(직선)"),
//    Homing     UMETA(DisplayName = "추적(호밍)"),
//    Attached   UMETA(DisplayName = "부착형")
//};

USTRUCT(BlueprintType)
struct FSkillInfo {
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, Category = "Effect")
    TSubclassOf<UGameplayEffect> SkillEffectClass;

    UPROPERTY(EditDefaultsOnly)
    FGameplayTag StatTag; // 참조할 스탯 (예: Stat.AttackPower, Stat.AbilityPower)

    UPROPERTY(EditDefaultsOnly)
    float Coefficient;    // 계수 (예: 0.6, 1.2)

    UPROPERTY(EditAnywhere)
    float CoeffPerLevel;

    UPROPERTY(EditDefaultsOnly)
    float BaseDamage;     // 스킬 기본 데미지

    UPROPERTY(EditDefaultsOnly)
    float ValuePerLevel; // 레벨별 스킬 기본 데미지 증가량
};

UCLASS()
class PROJECTER_API USkillDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
    UPROPERTY(EditDefaultsOnly, Category = "Skill|Damage")
    ESkillActivationType SkillActivationType;

    UPROPERTY(EditDefaultsOnly, Category = "Skill|Damage")
    TArray<FSkillInfo> DamageInfos;
};
