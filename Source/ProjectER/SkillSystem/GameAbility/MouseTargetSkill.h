// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameAbility/SkillBase.h"
#include "MouseTargetSkill.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTER_API UMouseTargetSkill : public USkillBase
{
	GENERATED_BODY()
public:
	UMouseTargetSkill();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	AActor* GetTargetUnderCursorInRange();
protected:
	virtual void ExecuteSkill() override;
	virtual void FinishSkill() override;
	void SetWaitTargetTask();
	AActor* GetTargetUnderCursor();
	bool IsInRange(AActor* Actor);
	void RotateToTarget(AActor* Actor);

	UFUNCTION()
	void OnTargetDataReady(const FGameplayAbilityTargetDataHandle& DataHandle);
	UFUNCTION()
	void OnTargetCancelled(const FGameplayAbilityTargetDataHandle& DataHandle);
private:

public:

protected:
	TSet<TObjectPtr<AActor>> AffectedActors;
private:
};
