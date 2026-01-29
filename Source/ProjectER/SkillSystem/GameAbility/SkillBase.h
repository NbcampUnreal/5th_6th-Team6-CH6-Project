// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
//#include "SkillSystem/SkillData.h"
#include "SkillBase.generated.h"

/**
 * 
 */
class USkillDataAsset;
class UBaseSkillConfig;

UCLASS()
class PROJECTER_API USkillBase : public UGameplayAbility
{
	GENERATED_BODY()

public:
	USkillBase();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	void Assign(USkillDataAsset* DataAsset);
protected:
	//virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void ExecuteSkill();
	virtual void FinishSkill();
	virtual void OnCancelAbility();
	void Instant();
	/*void Targeted();
	void OnTargetConfirmed(float ElapsedTime);*/
	/*virtual void PointClick();
	virtual void ClickAndDrag();
	virtual void Holding();*/
	void AddTagToOwner(FGameplayTag Tag);
	void RemoveTagFromOwner(FGameplayTag Tag);
	void OnActiveTagAdded();
	void PlayAnimMontage();
	void SetWaitActiveTagTask();
	void PrepareToActiveSkill();
//private:

public:

protected:
	/*UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Skill|Data")
	FSkillDefaultData SkillData;*/

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config")
	TObjectPtr<UBaseSkillConfig> SkillConfig;

	UPROPERTY(VisibleAnywhere, Category = "Skill|Tags")
	FGameplayTag CastingTag;

	UPROPERTY(VisibleAnywhere, Category = "Skill|Tags")
	FGameplayTag ActiveTag;

	UPROPERTY(VisibleAnywhere, Category = "Skill|Tags")
	FGameplayTag StatusTag;
//private:
	//FGameplayAttribute
};
