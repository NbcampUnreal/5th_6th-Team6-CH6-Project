// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillBase.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "SkillSystem/AbilityTask/AbilityTask_WaitGameplayEventSyn.h"
#include "SkillSystem/SkillConfig/BaseSkillConfig.h"
#include "SkillSystem/SkillDataAsset.h"
#include "SkillSystem/GameplyeEffect/SkillEffectDataAsset.h"
#include "Abilities/GameplayAbilityTargetActor.h"

#include "AbilitySystemLog.h" // GAS 관련 로그 확인용

USkillBase::USkillBase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	CastingTag = FGameplayTag::RequestGameplayTag(FName("Skill.Animation.Casting"));
	ActiveTag = FGameplayTag::RequestGameplayTag(FName("Skill.Animation.Active"));
	StatusTag = FGameplayTag::RequestGameplayTag(FName("Skill.Data.IncomingStatus"));
}

void USkillBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	//ENetMode NetMode = (GetWorld() != nullptr) ? GetWorld()->GetNetMode() : NM_Standalone;

	//FString NetModeStr;
	//switch (NetMode)
	//{
	//case NM_Client: NetModeStr = TEXT("Client"); break;
	//case NM_DedicatedServer: NetModeStr = TEXT("DedicatedServer"); break;
	//case NM_ListenServer: NetModeStr = TEXT("ListenServer"); break;
	//case NM_Standalone: NetModeStr = TEXT("Standalone"); break;
	//default: NetModeStr = TEXT("Unknown"); break;
	//}
	//FString SideStr = HasAuthority(&ActivationInfo) ? TEXT("SERVER") : TEXT("CLIENT");
	//FString ActorName = (ActorInfo && ActorInfo->AvatarActor.IsValid()) ? ActorInfo->AvatarActor->GetName() : TEXT("Unknown");
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (CommitAbility(Handle, ActorInfo, ActivationInfo) == false)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
}

void USkillBase::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bWasCancelled == true)
	{
		OnCancelAbility();
	}

	RemoveTagFromOwner(CastingTag);
	RemoveTagFromOwner(ActiveTag);
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USkillBase::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	if (Spec.SourceObject.IsValid())
	{
		USkillDataAsset* SkillDataAsset = Cast<USkillDataAsset>(Spec.SourceObject);
		if (IsValid(SkillDataAsset))
		{
			CachedConfig = SkillDataAsset->SkillConfig;
		}
	}
}


//void USkillBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
//{
//	Super::PostEditChangeProperty(PropertyChangedEvent);
//}

void USkillBase::ExecuteSkill()
{
	if (IsValid(CachedConfig) == false || CachedConfig->GetExcutionEffects().Num() <= 0) return;

	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (IsValid(Avatar) == false) return;

	if (HasAuthority(&CurrentActivationInfo))
	{
		ApplyEffectsToActor(Avatar, CachedConfig->GetExcutionEffects());
	}

	if (IsLocallyControlled())
	{
		OnExecuteSkill_InClient();
	}
}

void USkillBase::OnActiveTagEventReceived(FGameplayEventData Payload)
{
	if (CachedConfig->Data.bIsUseCasting)
	{
		if (GetAbilitySystemComponentFromActorInfo()->HasMatchingGameplayTag(CastingTag) == false)
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
			return;
		}
		RemoveTagFromOwner(CastingTag);
	}

	ExecuteSkill();
}

void USkillBase::OnCastingTagEventReceived(FGameplayEventData Payload)
{
	AddTagToOwner(CastingTag);

	/*UAbilityTask_SendServerEvent* SendEvnet = UAbilityTask_SendServerEvent::SendServerEvent(this, CastingTag);
	SendEvnet->ReadyForActivation();*/
}

void USkillBase::PlayAnimMontage()
{
	UAbilityTask_PlayMontageAndWait* PlayTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, TEXT("SkillAnimation"), CachedConfig->Data.AnimMontage);
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
	SetWaitEventActiveTag();
	if (CachedConfig->Data.bIsUseCasting) SetWaitEventCastingTag();
	if (IsLocallyControlled()) PlayAnimMontage();
}

void USkillBase::ApplyEffectsToActors(TSet<TObjectPtr<AActor>> Actors, const TArray<TObjectPtr<USkillEffectDataAsset>>& SkillEffectDataAssets)
{
	if (Actors.Num() <= 0 || SkillEffectDataAssets.Num() <= 0) return;

	FGameplayAbilityTargetDataHandle TargetDataHandle;
	FGameplayAbilityTargetData_ActorArray* NewData = new FGameplayAbilityTargetData_ActorArray();

	for (AActor* Target : Actors)
	{
		if (IsValid(Target))
		{
			NewData->TargetActorArray.Add(Target);
		}
	}

	TargetDataHandle.Add(NewData);

	UAbilitySystemComponent* InstigatorASC = GetAbilitySystemComponentFromActorInfo();
	for (USkillEffectDataAsset* EffectData : SkillEffectDataAssets)
	{
		if (!EffectData) continue;
		TArray<FGameplayEffectSpecHandle> SpecHandles = EffectData->MakeSpecs(InstigatorASC, this, GetAvatarActorFromActorInfo());
		for (FGameplayEffectSpecHandle& SpecHandle : SpecHandles)
		{
			if (SpecHandle.IsValid())
			{
				ApplyGameplayEffectSpecToTarget(
					CurrentSpecHandle,
					CurrentActorInfo,
					CurrentActivationInfo,
					SpecHandle,
					TargetDataHandle
				);
			}
		}
	}
}

void USkillBase::ApplyEffectsToActor(AActor* Actors, const TArray<TObjectPtr<USkillEffectDataAsset>>& SkillEffectDataAssets)
{
	if (IsValid(Actors) == false) return;

	TSet<TObjectPtr<AActor>> TargetSet;
	TargetSet.Add(Actors);

	ApplyEffectsToActors(TargetSet, SkillEffectDataAssets);
}

FGameplayTag USkillBase::GetInputTag()
{
	return CachedConfig->Data.InputKeyTag;
}

void USkillBase::FinishSkill()
{
	RemoveTagFromOwner(ActiveTag);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USkillBase::OnCancelAbility()
{

}

void USkillBase::OnExecuteSkill_InClient()
{
}

void USkillBase::AddTagToOwner(FGameplayTag Tag)
{
	if (Tag.IsValid())
	{
		//같은 태그가 여러개 있어도 해당 태그를 1개로 설정
		//GetAbilitySystemComponentFromActorInfo()->AddLooseGameplayTag(Tag, 1);
		GetAbilitySystemComponentFromActorInfo()->SetTagMapCount(Tag, 1);
	}
}

void USkillBase::RemoveTagFromOwner(FGameplayTag Tag)
{
	if (Tag.IsValid())
	{
		//같은 태그가 여러개 있어도 해당 태그를 0개로 설정
		//GetAbilitySystemComponentFromActorInfo()->RemoveLooseGameplayTag(Tag, 1);
		GetAbilitySystemComponentFromActorInfo()->SetTagMapCount(Tag, 0);
	}
}