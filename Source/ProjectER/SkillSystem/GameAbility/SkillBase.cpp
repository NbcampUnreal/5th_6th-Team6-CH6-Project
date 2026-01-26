// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillBase.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

USkillBase::USkillBase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	CastingTag = FGameplayTag::RequestGameplayTag(FName("Skill.Animation.Casting"));
	ActiveTag = FGameplayTag::RequestGameplayTag(FName("Skill.Animation.Active"));

	//AbilityTriggers.Add(SkillData.InputKeyTag);
}

void USkillBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	GetCooldownGameplayEffect();
	if (CommitAbility(Handle, ActorInfo, ActivationInfo) == false)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 예측 시작 구간 (이 아래의 로직은 클라이언트에서 선행 실행됨)
	if (SkillData.bIsUseCasting)
	{
		// 1. 태스크 생성 및 델리게이트 연결 (ReadyForActivation 전 수행)
		UAbilityTask_PlayMontageAndWait* PlayTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, TEXT("SkillAnimation"), SkillData.AnimMontage);
		PlayTask->ReadyForActivation();

		UAbilityTask_WaitGameplayTagAdded* WaitTagAdd = UAbilityTask_WaitGameplayTagAdded::WaitGameplayTagAdd(this, ActiveTag);
		WaitTagAdd->Added.AddDynamic(this, &USkillBase::OnActiveTagAdded);
		WaitTagAdd->ReadyForActivation();
		AddTagToOwner(CastingTag);
	}
	else
	{
		AddTagToOwner(ActiveTag);
		ExecuteSkill();
	}
}

void USkillBase::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bWasCancelled == true)
	{
		OnCancelAbility();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

//void USkillBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
//{
//	Super::PostEditChangeProperty(PropertyChangedEvent);
//}

void USkillBase::ExecuteSkill()
{
	//FinishSkill();
}

void USkillBase::OnActiveTagAdded()
{
	bool IsUseCasting = SkillData.bIsUseCasting;
	if (IsUseCasting && GetAbilitySystemComponentFromActorInfo()->HasMatchingGameplayTag(CastingTag))
	{
		RemoveTagFromOwner(CastingTag);
		ExecuteSkill();
	}
	else {
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void USkillBase::FinishSkill()
{
	RemoveTagFromOwner(ActiveTag);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USkillBase::OnCancelAbility()
{

}

void USkillBase::AddTagToOwner(FGameplayTag Tag)
{
	if (Tag.IsValid())
	{
		GetAbilitySystemComponentFromActorInfo()->AddLooseGameplayTag(Tag);
	}
}

void USkillBase::RemoveTagFromOwner(FGameplayTag Tag)
{
	if (Tag.IsValid())
	{
		GetAbilitySystemComponentFromActorInfo()->RemoveLooseGameplayTag(Tag);
	}
}