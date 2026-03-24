#include "Monster/GAS/GA/GA_MonsterState_Alert.h"

UGA_MonsterState_Alert::UGA_MonsterState_Alert()
{
	StateInitData.MonsterAssetTags = FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Ability.Action.Alert"));
	StateInitData.MontageType = EMonsterMontageType::Alert;
	StateInitData.NiagaraCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Particle.Action.Alert");
	StateInitData.SoundCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Sound.Action.Alert");
	StateInitData.WaitTag = FGameplayTag::RequestGameplayTag("State.Action.Alert");
	bIsUseWaitTag = true;
	SetAssetTags(StateInitData.MonsterAssetTags);
}

void UGA_MonsterState_Alert::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);
}

void UGA_MonsterState_Alert::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGA_MonsterState_Alert::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_MonsterState_Alert::OnMontageCompleted()
{
}

void UGA_MonsterState_Alert::OnMontageBlendIn()
{
}

void UGA_MonsterState_Alert::OnMontageBlendOut()
{
}

void UGA_MonsterState_Alert::OnMontageInterrupt()
{
}

void UGA_MonsterState_Alert::OnMontageCancel()
{
}

void UGA_MonsterState_Alert::OnTagRemoved()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
