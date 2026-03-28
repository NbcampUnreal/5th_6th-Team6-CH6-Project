#include "Monster/GAS/GA/GA_MonsterState_Death.h"
#include "Monster/BaseMonster.h"
#include "GameFramework/CharacterMovementComponent.h"

UGA_MonsterState_Death::UGA_MonsterState_Death()
{
	StateInitData.MonsterAssetTags = FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Ability.Action.Death"));
	StateInitData.MontageType = EMonsterMontageType::Dead;
	StateInitData.NiagaraCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Particle.Action.Death");
	StateInitData.SoundCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Sound.Action.Death");
	StateInitData.WaitTag = FGameplayTag::RequestGameplayTag("State.Life.Death");
	bIsUseWaitTag = false;
	SetAssetTags(StateInitData.MonsterAssetTags);
}

void UGA_MonsterState_Death::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);
}


void UGA_MonsterState_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ABaseMonster* Monster = Cast<ABaseMonster>(GetOwningActorFromActorInfo());
	if (IsValid(Monster) == false || IsValid(Monster->MonsterData) == false)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UCharacterMovementComponent* MoveComp = Monster->GetCharacterMovement();
	if (IsValid(MoveComp) == false)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	MoveComp->StopMovementImmediately();
	MoveComp->DisableMovement();
	
	if (Monster->HasAuthority())
	{
		Monster->Multicast_SetCollisionProfileName(FName("Spectator"));
	}
}

void UGA_MonsterState_Death::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, true, false);

}




