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

    UPROPERTY(EditDefaultsOnly, Category = "Status|Coefficient")
    FScalableFloat Coefficient;// 계수 (예: 0.6, 1.2)

    UPROPERTY(EditDefaultsOnly, Category = "Status")
    FScalableFloat BasedValue;//기본값

    UPROPERTY(EditDefaultsOnly, Category = "Skill|Status")
    FGameplayAttribute SourceAttribute; //계수값을 곱할 스텟값

    UPROPERTY(EditDefaultsOnly, Category = "Status|Attribute")
    FGameplayAttribute TargetAttribute; //계산한 값을가지고 반영할 상대의 목표 스탯
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

    UPROPERTY(EditDefaultsOnly, Category = "Skill")
    ESkillActivationType SkillActivationType;

    UPROPERTY(EditDefaultsOnly, Category = "Skill")
    bool bIsUseCasting;

    UPROPERTY(EditDefaultsOnly, Category = "Skill|Animation")
    TObjectPtr<UAnimMontage> AnimMontage;

    UPROPERTY(EditDefaultsOnly, Category = "Skill|Status")
    FScalableFloat BaseCoolTime;
};

//class PROJECTER_API SkillData
//{
//public:
//	SkillData();
//	~SkillData();
//};
