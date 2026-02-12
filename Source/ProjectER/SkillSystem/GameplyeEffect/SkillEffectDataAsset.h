// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayEffect.h"
#include "BaseGameplayEffect.h"
#include "SkillEffectDataAsset.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FSkillAttributeData {
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly)
    FScalableFloat Coefficients;//계수

    UPROPERTY(EditDefaultsOnly)
    FScalableFloat BasedValues;//기본값

    UPROPERTY(EditDefaultsOnly)
    FGameplayAttribute SourceAttribute; //계수값을 곱할 스텟값
};

USTRUCT(BlueprintType)
struct FSkillEffectDefinition {
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<UBaseGameplayEffect> SkillEffectClass;

    UPROPERTY(EditDefaultsOnly)
    TArray<FSkillAttributeData> SkillAttributeData;
};

USTRUCT(BlueprintType)
struct FSkillEffectContainer {
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly)
    TArray<FSkillEffectDefinition> SkillEffectDefinition;

    UPROPERTY(EditDefaultsOnly)
    FGameplayAttribute TargetAttribute; //계산한 값을가지고 반영할 상대의 목표 스탯
};

class UAbilitySystemComponent;
class USkillBase;

UCLASS()
class PROJECTER_API USkillEffectDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
    USkillEffectDataAsset();

    TArray<FGameplayEffectSpecHandle> MakeSpecs(UAbilitySystemComponent* InstigatorASC, USkillBase* InstigatorSkill, AActor* InEffectCauser = nullptr);
    FORCEINLINE FGameplayTag GetIndexTag() const { return IndexTag; }
    FORCEINLINE FSkillEffectContainer GetData() const { return Data; }
    FORCEINLINE FGameplayAttribute GetTargetAttribute() const { return Data.TargetAttribute; }
protected:
    
private:

public:
    

protected:

private:
    UPROPERTY(EditDefaultsOnly)
    FSkillEffectContainer Data;

    UPROPERTY(EditDefaultsOnly)
    FGameplayTag IndexTag;
};
