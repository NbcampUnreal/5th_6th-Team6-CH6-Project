// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayEffect.h"
#include "GameplyeEffect/BaseGameplayEffect.h"
#include "SkillData.generated.h"

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
enum class ETargetRelationship : uint8 {
    None UMETA(DisplayName = "None", Hidden),
    Friend     UMETA(DisplayName = "Friend"),
    Enemy   UMETA(DisplayName = "Enemy")
    //FriendAndEnemy   UMETA(DisplayName = "EnemyAndFriend")
};

class UAnimMontage; 

USTRUCT(BlueprintType)
struct FSkillDefaultData {
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, Category = "Skill")
    ESkillActivationType SkillActivationType;

    UPROPERTY(EditDefaultsOnly, Category = "Skill")
    ETargetRelationship ApplyTo = ETargetRelationship::None;

    UPROPERTY(EditDefaultsOnly, Category = "Skill")
    bool bIsUseCasting;

    UPROPERTY(EditDefaultsOnly, Category = "Skill|Animation")
    TObjectPtr<UAnimMontage> AnimMontage;

    UPROPERTY(EditDefaultsOnly, Category = "Skill|Status")
    FScalableFloat BaseCoolTime;

    UPROPERTY(EditDefaultsOnly, Category = "Skill|InputKey", meta = (Categories = "Input"))
    FGameplayTag InputKeyTag;

};

//class PROJECTER_API SkillData
//{
//public:
//	SkillData();
//	~SkillData();
//};
