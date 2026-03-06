// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameAbility/SkillBase.h"
#include "MouseTargetSkill.generated.h"

/**
 * 
 */

class ATargetActor;

UCLASS()
class PROJECTER_API UMouseTargetSkill : public USkillBase
{
	GENERATED_BODY()
public:
	UMouseTargetSkill();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	AActor* GetTargetUnderCursorInRange();
	bool IsTargetActorInRange(AActor* InTargetActor);
protected:
	virtual void ExecuteSkill() override;
	virtual void CompleteFinishSkill() override;
	virtual void OnCancelAbility() override;
	void SetWaitTargetTask();
	void SetWaitExternalTargetEventTask();
	void SubmitExternalTargetActor(AActor* InTargetActor);
	bool ConsumePendingExternalTargetActor(AActor*& OutTargetActor);
	AActor* GetTargetUnderCursor();
	bool IsInRange(AActor* Actor);
	void RotateToTarget(AActor* Actor);
	void ApplyEffectsTarget(AActor* TargetActor, const TArray<TObjectPtr<USkillEffectDataAsset>>& SkillEffectDataAssets);
	void CleanUpSkill();

	UFUNCTION()
	void OnTargetDataReady(const FGameplayAbilityTargetDataHandle& DataHandle);
	UFUNCTION()
	void OnTargetCancelled(const FGameplayAbilityTargetDataHandle& DataHandle);
	UFUNCTION()
	void OnExternalTargetActorReceived(FGameplayEventData Payload);
private:

public:

protected:
	TWeakObjectPtr<AActor> AffectedActor;
	TWeakObjectPtr<ATargetActor> CurrentTargetActor;
	TWeakObjectPtr<AActor> PendingExternalTargetActor;
	FGameplayTag ExternalTargetActorEventTag;
private:
};
