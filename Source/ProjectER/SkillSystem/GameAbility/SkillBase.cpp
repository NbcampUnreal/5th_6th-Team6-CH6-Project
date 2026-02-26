// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillBase.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "SkillSystem/AbilityTask/AbilityTask_WaitGameplayEventSyn.h"
#include "SkillSystem/SkillConfig/BaseSkillConfig.h"
#include "SkillSystem/SkillDataAsset.h"
#include "SkillSystem/SkillData.h"
#include "SkillSystem/GameplyeEffect/SkillEffectDataAsset.h"
#include "SkillSystem/GameplyeEffect/GE_SharedCooldown.h"
#include "Monster/BaseMonster.h"
#include "CharacterSystem/Character/BaseCharacter.h"
#include "CharacterSystem/Interface/TargetableInterface.h"
#include "GameModeBase/State/ER_PlayerState.h"

#include "AbilitySystemLog.h" // GAS 관련 로그 확인용

USkillBase::USkillBase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	CastingTag = FGameplayTag::RequestGameplayTag(FName("Skill.Animation.Casting"));
	ActiveTag = FGameplayTag::RequestGameplayTag(FName("Skill.Animation.Active"));
	//ActivationBlockedTags.AddTag(CastingTag);
	ActivationBlockedTags.AddTag(ActiveTag);

	CooldownGameplayEffectClass = UGE_SharedCooldown::StaticClass();
}

void USkillBase::SetSkillTagCount(FGameplayTag Tag, int32 Count)
{
	if (Tag.IsValid() && GetASC()) GetASC()->SetTagMapCount(Tag, Count);
}

void USkillBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void USkillBase::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bWasCancelled) {
		OnCancelAbility();
	}
	SetSkillTagCount(CastingTag, 0);
	SetSkillTagCount(ActiveTag, 0);
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USkillBase::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	USkillDataAsset* DataAsset = Cast<USkillDataAsset>(Spec.SourceObject);
	CachedConfig = IsValid(DataAsset) ? DataAsset->SkillConfig : nullptr;
}

//void USkillBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
//{
//	Super::PostEditChangeProperty(PropertyChangedEvent);
//}

void USkillBase::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();

	if (CooldownGE && CachedConfig)
	{
		FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CooldownGE->GetClass(), GetAbilityLevel());

		if (SpecHandle.IsValid())
		{
			float Duration = CachedConfig->Data.BaseCoolTime.GetValueAtLevel(GetAbilityLevel());
			SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Skill.Data.CoolTime")), Duration);
			SpecHandle.Data.Get()->DynamicGrantedTags.AppendTags(CachedConfig->Data.CoolTimeTags);
			ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
		}
	}
}

const FGameplayTagContainer* USkillBase::GetCooldownTags() const
{
	// 데이터 에셋에 쿨타임 태그가 설정되어 있다면 그것을 우선적으로 반환합니다.
	if (CachedConfig && CachedConfig->Data.CoolTimeTags.IsValid())
	{
		return &CachedConfig->Data.CoolTimeTags;
	}

	return nullptr;
}

void USkillBase::ExecuteSkill()
{
	auto* ASC = GetASC();
	auto* Avatar = GetAvatar();
	if (!IsValid(CachedConfig) || !IsValid(ASC) || !IsValid(Avatar))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// 메인 로직: 들여쓰기 없이 평탄하게 진행
	SetSkillTagCount(ActiveTag, 1);
	ApplyEffectsToActor(Avatar, CachedConfig->GetExcutionEffects());

	if (HasAuthority(&CurrentActivationInfo))
	{
		if (auto* Character = Cast<ABaseCharacter>(Avatar)) Character->StopMove();
	}

	if (IsLocallyControlled()) OnExecuteSkill_InClient();
}

void USkillBase::OnActiveTagEventReceived(FGameplayEventData Payload)
{
	if (!TryExecuteSkill())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void USkillBase::OnCastingTagEventReceived(FGameplayEventData Payload)
{
	if (CachedConfig && CachedConfig->Data.bIsUseCasting)
	{
		SetSkillTagCount(CastingTag, 1);
	}
}

void USkillBase::OnMontageInterrupted()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void USkillBase::OnMontageCancelled()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void USkillBase::OnMontageCompleted()
{
	CompleteFinishSkill();
}

void USkillBase::PlayAnimMontage()
{
	if (!IsValid(CachedConfig) || !IsValid(CachedConfig->Data.AnimMontage)) return;
	UAbilityTask_PlayMontageAndWait* PlayTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, TEXT("SkillAnimation"), CachedConfig->Data.AnimMontage);
	if (!IsValid(PlayTask)) return;

	PlayTask->OnInterrupted.AddDynamic(this, &USkillBase::OnMontageInterrupted);
	PlayTask->OnCancelled.AddDynamic(this, &USkillBase::OnMontageCancelled);
	PlayTask->OnCompleted.AddDynamic(this, &USkillBase::OnMontageCompleted);
	PlayTask->ReadyForActivation();
}

void USkillBase::SetWaitEventActiveTag()
{
	UAbilityTask_WaitGameplayEventSyn* WaitEventTask = UAbilityTask_WaitGameplayEventSyn::WaitEventClientToServer(this, ActiveTag);
	WaitEventTask->OnEventReceived.AddDynamic(this, &USkillBase::OnActiveTagEventReceived);
	WaitEventTask->ReadyForActivation();
}

void USkillBase::SetWaitEventCastingTag()
{
	UAbilityTask_WaitGameplayEventSyn* WaitEventTask = UAbilityTask_WaitGameplayEventSyn::WaitEventClientToServer(this, CastingTag);
	WaitEventTask->OnEventReceived.AddDynamic(this, &USkillBase::OnCastingTagEventReceived);
	WaitEventTask->ReadyForActivation();
}

void USkillBase::PrepareToActiveSkill()
{
	if (!IsValid(CachedConfig)) return;

	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	SetWaitEventActiveTag();
	if (CachedConfig->Data.bIsUseCasting) SetWaitEventCastingTag();

	if (IsLocallyControlled() || HasAuthority(&CurrentActivationInfo)) PlayAnimMontage();
}

void USkillBase::ApplyEffectsToActors(TSet<TObjectPtr<AActor>> Actors, const TArray<TObjectPtr<USkillEffectDataAsset>>& SkillEffectDataAssets, const FGameplayEffectContextHandle InEffectContextHandle)
{
	auto* ASC = GetASC();
	if (!ASC || Actors.Num() <= 0 || SkillEffectDataAssets.Num() <= 0) return;

	// 1. 타겟 데이터 생성 (인라인 루프)
	auto* Data = new FGameplayAbilityTargetData_ActorArray();
	for (AActor* Target : Actors) {
		if (IsValidRelationship(Target)) {
			Data->TargetActorArray.Add(Target);
		}
	}

	FGameplayAbilityTargetDataHandle Handle(Data);

	// 2. 이펙트 순차 적용
	for (USkillEffectDataAsset* Effect : SkillEffectDataAssets)
	{
		if (!Effect) continue;
		for (auto& Spec : Effect->MakeSpecs(ASC, this, GetAvatar(), InEffectContextHandle))
		{
			if (Spec.IsValid()) ApplyGameplayEffectSpecToTarget(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, Spec, Handle);
		}
	}
}

void USkillBase::ApplyEffectsToActor(AActor* Actor, const TArray<TObjectPtr<USkillEffectDataAsset>>& SkillEffectDataAssets, const FGameplayEffectContextHandle InEffectContextHandle)
{
	ApplyEffectsToActors({Actor}, SkillEffectDataAssets, InEffectContextHandle);
}

bool USkillBase::TryExecuteSkill()
{
	auto* ASC = GetASC();
	if (!IsValid(ASC) || !IsValid(CachedConfig)) {
		return false;
	}

	if (CachedConfig->Data.bIsUseCasting && ASC->HasMatchingGameplayTag(CastingTag) == false){
		return false;
	}

	if (!DoesAbilitySatisfyTagRequirements(*ASC, nullptr, nullptr, nullptr)){
		return false;
	}

	SetSkillTagCount(CastingTag, 0);
	ExecuteSkill();
	return true;
}

void USkillBase::CompleteFinishSkill()
{
	SetSkillTagCount(ActiveTag, 0);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

FGameplayTag USkillBase::GetInputTag()
{
	return CachedConfig ? CachedConfig->Data.InputKeyTag : FGameplayTag();
}

ETargetRelationship USkillBase::GetSkillTargetRelationship()
{
	return CachedConfig ? CachedConfig->Data.ApplyTo : ETargetRelationship::None;
}

bool USkillBase::IsValidRelationship(AActor* Target)
{
	if (!IsValid(Target) || !IsValid(CachedConfig)) return false;

	auto* Instigator = GetAvatar();
	auto* I_Instigator = Cast<ITargetableInterface>(Instigator);
	auto* I_Target = Cast<ITargetableInterface>(Target);

	bool IsInstigatorImplementsInterface = Instigator->GetClass()->ImplementsInterface(UTargetableInterface::StaticClass());
	bool TargetImplementsInterface = Target->GetClass()->ImplementsInterface(UTargetableInterface::StaticClass());

	if (!IsInstigatorImplementsInterface || !TargetImplementsInterface)
	{
		return false;
	}

	//if (!IsValid(I_Instigator) || !IsValid()) return false;
	if (!I_Instigator || !I_Target) return false;

	bool bIsSameTeam = (I_Instigator->GetTeamType() == I_Target->GetTeamType());
	const ETargetRelationship& Relationship = CachedConfig->Data.ApplyTo;

	if (Relationship == ETargetRelationship::Friend) return bIsSameTeam;
	if (Relationship == ETargetRelationship::Enemy)  return !bIsSameTeam && I_Target->IsTargetable();

	return false;
}

void USkillBase::OnCancelAbility()
{

}

void USkillBase::OnExecuteSkill_InClient()
{

}