// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectComponent/MoveBaseGEC.h"
#include "LaunchMoveGEC.generated.h"

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API ULaunchMoveGECConfig : public UMoveBaseConfig
{
	GENERATED_BODY()

public:
	// 수직 발사 속도 (Z축)
	// - 0보다 크면 '도약'으로 처리되어 MoveDistance 지점에 정확히 착지하도록 수평 속도가 계산됩니다.
	// - 0이면 '지면 대쉬'로 처리됩니다.
	UPROPERTY(EditDefaultsOnly, Category = "Launch")
	float VerticalLaunchSpeed = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Launch")
	bool bXYOverride = true;

	UPROPERTY(EditDefaultsOnly, Category = "Launch")
	bool bZOverride = true;
};

UCLASS()
class PROJECTER_API ULaunchMoveGEC : public UMoveBaseGEC
{
	GENERATED_BODY()

public:
	ULaunchMoveGEC();
	virtual TSubclassOf<UBaseGECConfig> GetRequiredConfigClass() const override;

	virtual float CalculateMoveDuration(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const FVector& Direction, const UMoveBaseConfig* Config) const override;

protected:
	virtual void Execute(AActor* Instigator, const FVector& Direction, const UMoveBaseConfig* Config, const FGameplayEffectSpec& GESpec) const override;
};
