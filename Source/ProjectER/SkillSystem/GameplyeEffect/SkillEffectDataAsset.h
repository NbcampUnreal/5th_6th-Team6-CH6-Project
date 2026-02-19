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
class UBaseGECConfig;

 UENUM(BlueprintType)
 enum class EAdjustmentType : uint8 {
     Add      UMETA(DisplayName = "Add"),
     Subtract UMETA(DisplayName = "Subtract")
 };

 UENUM(BlueprintType)
     enum class EDecreaseBy : uint8 {
     Noting     UMETA(DisplayName = "True"),
     Defense    UMETA(DisplayName = "Noraml"),
     Tenacity   UMETA(DisplayName = "DecreaseStatus")
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
    EAdjustmentType Adjustment = EAdjustmentType::Add;

    UPROPERTY(EditDefaultsOnly)
    EDecreaseBy DamageType = EDecreaseBy::Defense;

    UPROPERTY(VisibleAnywhere, Instanced)
    TObjectPtr<UBaseGECConfig> Config;
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

    TArray<FGameplayEffectSpecHandle> MakeSpecs(UAbilitySystemComponent* InstigatorASC, USkillBase* InstigatorSkill, AActor* InEffectCauser, const FGameplayEffectContextHandle InEffectContextHandle = FGameplayEffectContextHandle()) const;
    FORCEINLINE FGameplayTag GetIndexTag() const { return IndexTag; }
    FORCEINLINE FSkillEffectContainer GetData() const { return Data; }
    FORCEINLINE FGameplayAttribute GetTargetAttribute() const { return Data.TargetAttribute; }
protected:
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    void RefreshConfigsFromGE();
private:

public:
    

protected:

private:
    UPROPERTY(EditDefaultsOnly)
    FSkillEffectContainer Data;

    UPROPERTY(VisibleAnywhere)
    FGameplayTag IndexTag;
};
