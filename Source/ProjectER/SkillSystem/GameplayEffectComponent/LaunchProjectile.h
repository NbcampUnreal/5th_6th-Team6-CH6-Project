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
	/** 발사체 기본 설정 */
	UPROPERTY(EditDefaultsOnly, Category = "Projectile | Base")
	TSubclassOf<ABaseRangeOverlapEffectActor> ProjectileClass; // 발사체 본체 BP

	UPROPERTY(EditDefaultsOnly, Category = "Projectile | Base")
	float LifeSpan = 5.0f; // 목표를 못 맞춰도 사라질 시간

	/** 이동 속성 */
	UPROPERTY(EditDefaultsOnly, Category = "Projectile | Movement")
	float InitialSpeed = 2000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile | Movement")
	float GravityScale = 0.0f; // 0이면 직선으로 날아감

	/** 충돌 로직 */
	UPROPERTY(EditDefaultsOnly, Category = "Projectile | Collision")
	bool bDestroyOnHit = true; // 적중 시 제거 (관통이면 false)

	UPROPERTY(EditDefaultsOnly, Category = "Projectile | Collision")
	FVector CollisionSize = FVector(20.0f); // 발사체 자체의 크기

	UPROPERTY(EditDefaultsOnly, Category = "Projectile | Spawn")
	FName BoneName;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile | Spawn")
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile | Spawn")
	float ZOffset = 0.0f;

	/** 적중 시 적용할 이펙트 */
	UPROPERTY(EditDefaultsOnly, Category = "Projectile | Effects")
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
