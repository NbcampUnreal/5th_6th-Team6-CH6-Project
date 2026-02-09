// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SkillBase.generated.h"

/**
 * 
 */

enum class ETargetRelationship : uint8;
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
	virtual void OnExecuteSkill_InClient();
	void AddTagToOwner(FGameplayTag Tag);
	void RemoveTagFromOwner(FGameplayTag Tag);
	void PlayAnimMontage();
	void SetWaitEventActiveTag();
	void SetWaitEventCastingTag();
	void PrepareToActiveSkill();
	void ApplyEffectsToActors(TSet<TObjectPtr<AActor>> Actors, const TArray<TObjectPtr<USkillEffectDataAsset>>& SkillEffectDataAssets);
	void ApplyEffectsToActor(AActor* Actors, const TArray<TObjectPtr<USkillEffectDataAsset>>& SkillEffectDataAssets);
	FGameplayTag GetInputTag();
	ETargetRelationship GetSkillTargetRelationship();
	bool IsValidRelationship(AActor* Target);

	UFUNCTION()
	void OnActiveTagEventReceived(FGameplayEventData Payload);

	UFUNCTION()
	void OnCastingTagEventReceived(FGameplayEventData Payload);
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
};
