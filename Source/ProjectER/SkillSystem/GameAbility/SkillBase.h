// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SkillBase.generated.h"

/**
 * 
 */
class USkillDataAsset;
class USkillEffectDataAsset;
class UBaseSkillConfig;

UCLASS()
class PROJECTER_API USkillBase : public UGameplayAbility
{
	GENERATED_BODY()

public:
	USkillBase();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
protected:
	//virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void ExecuteSkill();
	virtual void FinishSkill();
	virtual void OnCancelAbility();
	void AddTagToOwner(FGameplayTag Tag);
	void RemoveTagFromOwner(FGameplayTag Tag);
	void PlayAnimMontage();
	void SetWaitActiveTagTask();
	void PrepareToActiveSkill();
	void ApplyEffectsToActors(TSet<TObjectPtr<AActor>> Actors, const TArray<TObjectPtr<USkillEffectDataAsset>>& SkillEffectDataAssets);
	void ApplyEffectsToActor(AActor* Actors, const TArray<TObjectPtr<USkillEffectDataAsset>>& SkillEffectDataAssets);
	FGameplayTag GetInputTag();

	UFUNCTION()
	void OnActiveTagAdded();
//private:

public:

protected:
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UBaseSkillConfig> CachedConfig;

	UPROPERTY(VisibleAnywhere, Category = "Skill|Tags")
	FGameplayTag CastingTag;

	UPROPERTY(VisibleAnywhere, Category = "Skill|Tags")
	FGameplayTag ActiveTag;

	UPROPERTY(VisibleAnywhere, Category = "Skill|Tags")
	FGameplayTag StatusTag;
//private:
	//FGameplayAttribute
};
