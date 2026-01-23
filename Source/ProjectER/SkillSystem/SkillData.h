// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayEffect.h"
#include "SkillData.generated.h"

/**
 * 
 */


USTRUCT(BlueprintType)
struct FSkillEffectData{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, Category = "Effect")
    TSubclassOf<UGameplayEffect> SkillEffectClass;
    //GE가 스탯태그를 참조하여 수치를 만들어서 반영함. 
    //스킬이 발동시킬 GE와 기본값이 있어야함. 

    UPROPERTY(EditDefaultsOnly, Category = "Status|Tag")
    FGameplayTag StatTag; // 참조할 스탯 (예: Stat.AttackPower, Stat.AbilityPower)

    UPROPERTY(EditDefaultsOnly, Category = "Status|Coefficient")
    float Coefficient;    // 계수 (예: 0.6, 1.2)

    UPROPERTY(EditDefaultsOnly, Category = "Status|Coefficient")
    float CoeffPerLevel;

    UPROPERTY(EditDefaultsOnly, Category = "Status|Base")
    float BaseValue;     // 스킬 기본 데미지

    UPROPERTY(EditDefaultsOnly, Category = "Status|Base")
    float ValuePerLevel; // 레벨별 스킬 기본 데미지 증가량
     //스킬이 키입력시 시전시간이 존재하는지 여부 
};

UENUM(BlueprintType)
enum class ESkillActivationType : uint8 {
    Instant    UMETA(DisplayName = "Instant"),
    Targeted   UMETA(DisplayName = "Targeted"),
    PointClick UMETA(DisplayName = "PointClick"),
    ClickAndDrag       UMETA(DisplayName = "ClickAndDrag"),
    Holding    UMETA(DisplayName = "Holding")
};

class UAnimMontage; 

USTRUCT(BlueprintType)
struct FSkillDefaultData {
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, Category = "Skill|Damage")
    ESkillActivationType SkillActivationType;

    UPROPERTY(EditDefaultsOnly, Category = "Skill")
    bool bIsUseCasting;

    UPROPERTY(EditDefaultsOnly, Category = "Skill|Animation")
    TSubclassOf<UAnimMontage> Animation;

    UPROPERTY(EditDefaultsOnly, Category = "Skill|Effect")
    TArray<FSkillEffectData> EffectDatas;

    UPROPERTY(EditDefaultsOnly, Category = "Skill|Status")
    float BaseCoolTime; // 레벨별 스킬 기본 쿨타임

    UPROPERTY(EditDefaultsOnly, Category = "Skill|Status")
    float ReduceCoolTimePerLevel; // 레벨별 줄여드는 쿨타임량
};

//class PROJECTER_API SkillData
//{
//public:
//	SkillData();
//	~SkillData();
//};
