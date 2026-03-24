#include "Monster/GAS/GA/GA_MonsterState.h"
#include "Monster/BaseMonster.h"
#include "Monster/Data/MonsterDataAsset.h"
#include "AbilitySystemComponent.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UGA_MonsterState::UGA_MonsterState()
{
	//Input
	bReplicateInputDirectly = false;

	//Advanced
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ClientOrServer;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	bServerRespectsRemoteAbilityCancellation = false;
	bRetriggerInstancedAbility = false;
}

void UGA_MonsterState::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

}

void UGA_MonsterState::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);


	if (StateInitData.MontageType == EMonsterMontageType::None || StateInitData.WaitTag.IsValid() == false)
	{
		UE_LOG(LogTemp, Error, TEXT("UGA_MonsterState::ActivateAbility : [%s] 데이터 초기화 누락!"), *GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ABaseMonster* Monster = Cast<ABaseMonster>(GetOwningActorFromActorInfo());
	if (IsValid(Monster) == false || IsValid(Monster->MonsterData) == false)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	TObjectPtr<UAnimMontage> Montage = Monster->MonsterData->Montages.FindRef(StateInitData.MontageType);
	if (IsValid(Montage))
	{
		UAbilityTask_PlayMontageAndWait* WaitPlayMontageTask =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, "PlayMontageTask", Montage);

		WaitPlayMontageTask->ReadyForActivation();
	}

	if (StateInitData.SoundCueTag.IsValid())
	{
		FGameplayCueParameters Params;
		Params.Instigator = Monster;
		ActorInfo->AbilitySystemComponent->ExecuteGameplayCue(StateInitData.SoundCueTag, Params);
	}

	if (StateInitData.NiagaraCueTag.IsValid())
	{
		FGameplayCueParameters Params;
		Params.Instigator = Monster;
		ActorInfo->AbilitySystemComponent->ExecuteGameplayCue(StateInitData.NiagaraCueTag, Params);
	}

	if (StateInitData.WaitTag.IsValid())
	{
		if (bIsWaitTag)
		{
			UAbilityTask_WaitGameplayTagRemoved* WaitRemoveTask = UAbilityTask_WaitGameplayTagRemoved::WaitGameplayTagRemove(this, StateInitData.WaitTag);
			WaitRemoveTask->Removed.AddDynamic(this, &UGA_MonsterState::OnTagRemoved);
			WaitRemoveTask->ReadyForActivation();
		}
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UGA_MonsterState::OnTagRemoved()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_MonsterState::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
