// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectComponent/SummonRangeAtBone.h"
#include "AouraGEC.generated.h"

/**
 * AouraGEC 전용 설정 클래스
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API UAouraGECConfig : public USummonRangeByBoneGECConfig
{
	GENERATED_BODY()

public:
	UAouraGECConfig();

	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Periodic")
	float Period = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Periodic")
	bool bApplyImmediately = true;
};

/**
 * 캐릭터 본에 부착되어 지속시간 동안 주기적으로 효과를 적용하는 GEC
 */
UCLASS()
class PROJECTER_API UAouraGEC : public USummonRangeAtBone
{
	GENERATED_BODY()

public:
	UAouraGEC();

	virtual TSubclassOf<UBaseGECConfig> GetRequiredConfigClass() const override;

protected:
	virtual void InitializeRangeActor(ABaseRangeOverlapEffectActor* RangeActor, const USummonRangeBaseConfig* Config, AActor* Instigator, const FGameplayEffectContextHandle& Context, const FGameplayCueParameters& HitTargetVfxCueParameters, const FGameplayCueParameters& HitTargetSoundCueParameters) const override;
};
