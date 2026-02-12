// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameAbility/SkillBase.h"
#include "MouseClickSkill.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTER_API UMouseClickSkill : public USkillBase
{
	GENERATED_BODY()
public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	void SendLocationData(FVector TargetLocation);
protected:
	virtual void ExecuteSkill() override;
	void PrepareDataSetDelegate(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData);
	bool IsInRange(const FVector& Location);
	void RotateToLocation(const FVector& Location);
	FVector GetMouseLocation() const;
	UFUNCTION()
	void OnTargetDataReady(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ActivationTag);
	//UFUNCTION()
	//void OnTargetCancelled(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ActivationTag);
private:

public:

protected:
	FDelegateHandle TargetDataDelegateHandle;
private:
};
