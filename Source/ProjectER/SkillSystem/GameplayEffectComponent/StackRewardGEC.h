// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectComponent/BaseGEC.h"
#include "SkillSystem/GameplayEffectComponent/BaseGECConfig.h"
#include "StackRewardGEC.generated.h"

class UGameplayEffect;
class UAbilitySystemComponent;
struct FActiveGameplayEffectHandle;
class USkillEffectDataAsset;
 
struct FActiveGameplayEffectsContainer;
struct FActiveGameplayEffect;

USTRUCT(BlueprintType)
struct FStackRewardInfo
{
	GENERATED_BODY()

	// 보상이 발동할 스택 수치
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 StackCount = 0;

	// 지급할 보상 효과 데이터 에셋
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<USkillEffectDataAsset> SkillEffectDataAsset;

	// 보상을 적(피격자)에게 줄 것인지 선택 (true: 적에게 데미지/디버프 부여, false: 공격자에게 버프 부여)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bApplyToTarget = false;

	// 보상 지급 후 대상의 스택을 초기화(제거)할지 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bClearStack = false;

	// 시전자(공격자) 측에서 재생할 이펙트
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX")
	TObjectPtr<class USkillNiagaraSpawnConfig> InstigatorVfxConfig;

	// 발동 대상(피격자) 측에서 재생할 이펙트
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX")
	TObjectPtr<class USkillNiagaraSpawnConfig> TargetVfxConfig;

	// 시전자(공격자) 측에서 재생할 사운드
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<class USkillSoundSpawnConfig> InstigatorSoundConfig;

	// 발동 대상(피격자) 측에서 재생할 사운드
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<class USkillSoundSpawnConfig> TargetSoundConfig;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API UStackRewardGECConfig : public UBaseGECConfig
{
	GENERATED_BODY()

public:
	// 스택 수치별 보상 설정 목록
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "StackReward")
	TArray<FStackRewardInfo> Rewards;
};

UCLASS()
class PROJECTER_API UStackRewardGEC : public UBaseGEC
{
	GENERATED_BODY()

public:
	UStackRewardGEC();
	virtual TSubclassOf<UBaseGECConfig> GetRequiredConfigClass() const override;

	virtual bool OnActiveGameplayEffectAdded(FActiveGameplayEffectsContainer& ActiveGEContainer, FActiveGameplayEffect& ActiveGE) const override;

protected:
	void ProcessStackRewards(UAbilitySystemComponent* TargetASC, FActiveGameplayEffectHandle InHandle, int32 CurrentStack, const UStackRewardGECConfig* Config) const;
};
