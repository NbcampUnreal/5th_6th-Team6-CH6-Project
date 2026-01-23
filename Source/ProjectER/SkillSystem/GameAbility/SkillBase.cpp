// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillBase.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"

USkillBase::USkillBase()
{
	CastingTag = FGameplayTag::RequestGameplayTag(FName("Skill.Animation.Casting"));
	ActiveTag = FGameplayTag::RequestGameplayTag(FName("Skill.Animation.Active"));
}

void USkillBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 예측 시작 구간 (이 아래의 로직은 클라이언트에서 선행 실행됨)
	if (SkillData.bIsUseCasting)
	{
		// 1. 태스크 생성 및 델리게이트 연결 (ReadyForActivation 전 수행)
		UAbilityTask_WaitGameplayTagAdded* WaitTagAdd = UAbilityTask_WaitGameplayTagAdded::WaitGameplayTagAdd(this, ActiveTag);
		WaitTagAdd->Added.AddDynamic(this, &USkillBase::OnActiveTagAdded);
		WaitTagAdd->ReadyForActivation();

		// [수정] LooseTag 대신 ReplicatedLooseTag 혹은 GE 사용 권장
		// 하지만 더 좋은 방법은 'AbilityTag' 설정을 이용해 실행 중 자동으로 붙게 하는 것입니다.
		// 여기서는 수동 제어를 위해 Replicated 버전을 사용합니다.
		AddTagToOwner(CastingTag);
	}
	else
	{
		AddTagToOwner(ActiveTag);
		ExecuteSkill();
	}
}

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
		//Cancled
	}
}

void USkillBase::FinishSkill()
{
	RemoveTagFromOwner(ActiveTag);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
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