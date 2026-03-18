// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectComponent/BaseGEC.h"
#include "SkillSystem/GameplayEffectComponent/BaseGECConfig.h"
#include "GameplayTagContainer.h"
#include "LaunchHomingMissile.generated.h"

class ABaseMissileActor;
class USkillEffectDataAsset;
class USkillNiagaraSpawnConfig;
struct FGameplayEffectSpec;
struct FGameplayEffectContextHandle;
struct FActiveGameplayEffectsContainer;
struct FPredictionKey;

/**
 * 유도 미사일 전용 설정 클래스.
 * UBaseGECConfig를 직접 상속하여 필요한 필드만 정의합니다.
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API ULaunchHomingMissileConfig : public UBaseGECConfig
{
	GENERATED_BODY()

public:
	virtual FText BuildTooltipDescription(float InLevel) const override;

	//--- 미사일 액터 클래스 ---
	UPROPERTY(EditDefaultsOnly, Category = "Missile|Base")
	TSubclassOf<ABaseMissileActor> MissileActorClass;

	//--- Movement ---
	UPROPERTY(EditDefaultsOnly, Category = "Missile|Movement")
	float InitialSpeed = 1500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Missile|Movement")
	float MaxSpeed = 2000.0f;

	//--- Homing ---
	UPROPERTY(EditDefaultsOnly, Category = "Missile|Homing")
	float HomingAccelerationMagnitude = 30000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Missile|Homing", meta = (ToolTip = "이 거리 이내로 접근하면 적중으로 판정"))
	float ReachThreshold = 50.0f;

	//--- Gameplay ---
	UPROPERTY(EditDefaultsOnly, Category = "Missile|Gameplay")
	float LifeSpan = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Missile|Gameplay")
	bool bDestroyOnHit = true;

	UPROPERTY(EditDefaultsOnly, Category = "Missile|Gameplay")
	FName BoneName;

	UPROPERTY(EditDefaultsOnly, Category = "Missile|Effect")
	TArray<TObjectPtr<USkillEffectDataAsset>> Applied;

	//--- Niagara VFX ---
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Missile|Niagara")
	TObjectPtr<USkillNiagaraSpawnConfig> SummonerVfx;

	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Missile|Niagara")
	TObjectPtr<USkillNiagaraSpawnConfig> MissileVfx;

	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Missile|Niagara")
	TObjectPtr<USkillNiagaraSpawnConfig> ImpactVfx;
};

/**
 * 유도 미사일을 발사하는 GameplayEffectComponent.
 * UBaseGEC를 직접 상속하여 SummonRange 계열 종속성을 제거합니다.
 */
UCLASS()
class PROJECTER_API ULaunchHomingMissile : public UBaseGEC
{
	GENERATED_BODY()

public:
	ULaunchHomingMissile();

	virtual TSubclassOf<UBaseGECConfig> GetRequiredConfigClass() const override;

protected:
	virtual void OnGameplayEffectApplied(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const override;

	FTransform CalculateSpawnTransform(const ULaunchHomingMissileConfig* Config, const AActor* Instigator, const AActor* TargetActor) const;
	AActor* GetTargetActorFromContainer(FActiveGameplayEffectsContainer& ActiveGEContainer) const;
	void ExecuteVfx(const FGameplayEffectSpec& GESpec, const FGameplayEffectContextHandle& ContextHandle, AActor* Instigator, ABaseMissileActor* MissileActor, const ULaunchHomingMissileConfig* Config) const;
};
