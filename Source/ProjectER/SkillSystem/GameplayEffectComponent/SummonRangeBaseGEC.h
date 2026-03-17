// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectComponent/BaseGEC.h"
#include "SkillSystem/GameplayEffectComponent/BaseGECConfig.h"
#include "SkillSystem/SkillNiagaraSpawnSettings.h"
#include "SummonRangeBaseGEC.generated.h"

/**
 * 
 */
class ABaseRangeOverlapEffectActor;
class USkillEffectDataAsset;
struct FGameplayCueParameters;
struct FGameplayEffectContextHandle;

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, Abstract)
class PROJECTER_API USummonRangeBaseConfig : public UBaseGECConfig
{
	GENERATED_BODY()

public:
	virtual FText BuildTooltipDescription(float InLevel) const override;

	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Base")
	TSubclassOf<ABaseRangeOverlapEffectActor> RangeActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Base")
	float LifeSpan = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Base")
	FVector CollisionRadius = FVector(100.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Base")
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Rotation")
	FRotator RotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Snap")
	bool bSnapToGround = true;

	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Snap", meta = (EditCondition = "bSnapToGround"))
	float FloatingHeight = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Snap", meta = (EditCondition = "bSnapToGround"))
	bool bUseBoxExtentOffset = true;

	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Snap", meta = (EditCondition = "bSnapToGround"))
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Effect")
	bool bHitOncePerTarget = true;

	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Niagara")
	FSkillNiagaraSpawnSettings SummonerSpawnVfx;

	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Niagara")
	FSkillNiagaraSpawnSettings RangeSpawnVfx;

	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Niagara")
	FSkillNiagaraSpawnSettings HitTargetVfx;

	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Effect")
	TArray<TObjectPtr<USkillEffectDataAsset>> Applied;
};

UCLASS()
class PROJECTER_API USummonRangeBaseGEC : public UBaseGEC
{
	GENERATED_BODY()
protected:
	virtual void OnGameplayEffectApplied(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const override;
	virtual const USummonRangeBaseConfig* GetSummonConfig(const FGameplayEffectSpec& GESpec) const;
	virtual FTransform CalculateSpawnTransform(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const AActor* TargetActor) const;
	virtual bool ShouldProcessOnInstigator(const AActor* Instigator) const;

	virtual void ExecuteGameplayCues(const FGameplayEffectSpec& GESpec, const FGameplayEffectContextHandle& ContextHandle, AActor* EffectInstigator, ABaseRangeOverlapEffectActor* RangeActor, const FTransform& SpawnTransform, const USummonRangeBaseConfig* Config) const;
	virtual AActor* GetTargetActorFromContainer(FActiveGameplayEffectsContainer& ActiveGEContainer) const;

	FGameplayCueParameters BuildNiagaraCueParameters(const FGameplayEffectSpec& GESpec, const FGameplayTag& OriginalTag, const FGameplayEffectContextHandle& EffectContext, AActor* EffectCauser, const FVector& CueLocation, const UObject* SourceObject, const FVector& CueNormal = FVector::UpVector) const;
	virtual void InitializeRangeActor(ABaseRangeOverlapEffectActor* RangeActor, const USummonRangeBaseConfig* Config, AActor* Instigator, const FGameplayEffectContextHandle& Context, const FGameplayCueParameters& HitTargetCueParameters) const;
	virtual void SnapLocationToGround(FVector& InOutLocation, const USummonRangeBaseConfig* Config, const AActor* Instigator) const;
	virtual void ApplyCommonSpawnOptions(FVector& InOutLocation, FRotator& InOutRotation, const USummonRangeBaseConfig* Config, const AActor* Instigator) const;
};

