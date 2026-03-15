// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectComponent/SummonRangeAtBone.h"
#include "LaunchProjectile.generated.h"

class AProjectileBase;

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API ULaunchProjectileConfig : public USummonRangeByBoneGECConfig
{
	GENERATED_BODY()
public:

public:
	UPROPERTY(EditDefaultsOnly, Category = "Projectile | Movement")
	float InitialSpeed = 2000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile | Movement")
	float GravityScale = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile | Collision")
	bool bDestroyOnHit = true;

	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Rotation", meta = (EditCondition = "!bUseInstigatorRotation"))
	bool bUseEffectContextDirection = true;
};

UCLASS()
class PROJECTER_API ULaunchProjectile : public USummonRangeAtBone
{
	GENERATED_BODY()
public:
	ULaunchProjectile();

	virtual TSubclassOf<UBaseGECConfig> GetRequiredConfigClass() const override;

protected:
	virtual void InitializeRangeActor(ABaseRangeOverlapEffectActor* RangeActor, const USummonRangeBaseConfig* Config, AActor* Instigator, const FGameplayEffectContextHandle& Context, const FGameplayCueParameters& HitTargetCueParameters) const override;
	virtual FTransform CalculateSpawnTransform(const FGameplayEffectSpec& GESpec, const AActor* Instigator) const override;
};
