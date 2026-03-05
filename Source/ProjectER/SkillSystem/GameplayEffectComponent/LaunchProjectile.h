// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectComponent/BaseGEC.h"
#include "SkillSystem/GameplayEffectComponent/BaseGECConfig.h"
#include "LaunchProjectile.generated.h"

/**
 * 
 */
class ABaseRangeOverlapEffectActor;
class USkillEffectDataAsset;
struct FGameplayEffectContextHandle;

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API ULaunchProjectileConfig : public UBaseGECConfig
{
	GENERATED_BODY()

public:
	virtual FText BuildTooltipDescription(float InLevel) const override;

public:
	/** 소환할 액터 및 수명 설정 */
	UPROPERTY(EditDefaultsOnly, Category = "Range Actor", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<ABaseRangeOverlapEffectActor> RangeActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Range Actor", meta = (AllowPrivateAccess = "true"))
	float LifeSpan = 1.0f;

	/** 소환 위치 및 트랜스폼 관련 설정 */
	UPROPERTY(EditDefaultsOnly, Category = "Spawn Settings", meta = (AllowPrivateAccess = "true"))
	bool bUseBoneLocationSpawn = false;

	UPROPERTY(EditDefaultsOnly, Category = "Spawn Settings", meta = (EditCondition = "bUseBoneLocationSpawn", AllowPrivateAccess = "true"))
	FName BoneName;

	UPROPERTY(EditDefaultsOnly, Category = "Spawn Settings", meta = (AllowPrivateAccess = "true"))
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "Spawn Settings", meta = (AllowPrivateAccess = "true"))
	float ZOffset = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Spawn Settings", meta = (AllowPrivateAccess = "true"))
	FRotator SpawnRotation = FRotator::ZeroRotator;

	/** 충돌 및 이펙트 적용 로직 설정 */
	UPROPERTY(EditDefaultsOnly, Category = "Effect Logic", meta = (AllowPrivateAccess = "true"))
	FVector CollisionRadius = FVector(100.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Effect Logic", meta = (AllowPrivateAccess = "true"))
	bool bHitOnce = false;

	UPROPERTY(EditDefaultsOnly, Category = "Effect Logic", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<USkillEffectDataAsset>> Applied;
};

UCLASS()
class PROJECTER_API ULaunchProjectile : public UBaseGEC
{
	GENERATED_BODY()
public:
	ULaunchProjectile();

	virtual TSubclassOf<UBaseGECConfig> GetRequiredConfigClass() const override;
protected:

public:

protected:
};
