// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "BaseModMagnitudeCalculation.generated.h"

struct FSkillEffectDefinition;

/**
 * Duration/Infinite GameplayEffect에서 복합적인 수치 계산을 처리하기 위한 베이스 클래스입니다.
 * UBaseExecutionCalculation의 로직을 MMC 형식으로 이식했습니다.
 */
UCLASS()
class PROJECTER_API UBaseModMagnitudeCalculation : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UBaseModMagnitudeCalculation();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

protected:
	/** 캡처된 속성 맵에서 특정 속성의 값을 찾아 반환합니다. */
	float FindValueByAttribute(const FGameplayEffectSpec& Spec, const FGameplayAttribute& Attribute, const TMap<FGameplayAttribute, FGameplayEffectAttributeCaptureDefinition>& TargetMap) const;
	
	/** SkillEffectDefinition에 정의된 수식에 따라 최종 수치를 계산합니다. */
	float ReturnCalculatedValue(const FGameplayEffectSpec& Spec, const FSkillEffectDefinition& SkillEffectDefinition, const float Level, const TMap<FGameplayAttribute, FGameplayEffectAttributeCaptureDefinition>& SelectedMap) const;
};
