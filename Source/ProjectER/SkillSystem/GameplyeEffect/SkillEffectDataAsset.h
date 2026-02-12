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

UENUM(BlueprintType)
enum class EAdjustment : uint8 {
    Add    UMETA(DisplayName = "Add"),
    Subtract    UMETA(DisplayName = "Subtract")
};

UENUM(BlueprintType)
enum class EDecreaseBy : uint8 {
    Noting    UMETA(DisplayName = "Noting"),
    Defense    UMETA(DisplayName = "Defense"),
    Tenacity    UMETA(DisplayName = "Tenacity")
};

USTRUCT(BlueprintType)
struct FSkillAttributeData {
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly)
    FScalableFloat Coefficients;//계수

    UPROPERTY(EditDefaultsOnly)
    FScalableFloat BasedValues;//기본값

    UPROPERTY(EditDefaultsOnly)
    FGameplayAttribute SourceAttribute; //계수값을 곱할 Attribute값
};

USTRUCT(BlueprintType)
struct FSkillEffectDefinition {
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<UBaseGameplayEffect> SkillEffectClass;

    UPROPERTY(EditDefaultsOnly)
    TArray<FSkillAttributeData> SkillAttributeData;

    UPROPERTY(EditDefaultsOnly)
    EAdjustment Adjustment = EAdjustment::Add;//계산한 값을 Attribute에 더할지 뺄지 결정하는 enum

    //데미지 타입이라고 생각하는게 편함.
    //Noting은 트루 데미지, Defense는 데미지, Tenacity는 스탯 디버프용
    UPROPERTY(EditDefaultsOnly)
    EDecreaseBy DecreaseBy = EDecreaseBy::Noting;
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
