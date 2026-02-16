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
	//void SendLocationData(FVector TargetLocation);
	bool TryGetMouseLocationInRange(FVector& OutLocation) const;
protected:
	virtual void ExecuteSkill() override;
	//void PrepareDataSetDelegate(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData);
	bool IsInRange(const FVector& Location) const;
	void RotateToLocation(const FVector& Location);
	void SetWaitTargetTask();
	FVector GetMouseLocation() const;
	/*UFUNCTION()
	void OnTargetDataReady(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ActivationTag);*/
	UFUNCTION()
	void OnTargetDataReady(const FGameplayAbilityTargetDataHandle& DataHandle);
	UFUNCTION()
	void OnTargetCancelled(const FGameplayAbilityTargetDataHandle& DataHandle);
private:

public:

protected:
	FDelegateHandle TargetDataDelegateHandle;
	FVector FinalLocation;
private:
};
