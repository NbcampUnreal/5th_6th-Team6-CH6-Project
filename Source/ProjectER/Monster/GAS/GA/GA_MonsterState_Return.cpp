#include "Monster/GAS/GA/GA_MonsterState_Return.h"
#include "Monster/BaseMonster.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"

UGA_MonsterState_Return::UGA_MonsterState_Return()
{
	StateInitData.MonsterAssetTags = FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Ability.Action.Return"));
	StateInitData.MontageType = EMonsterMontageType::Move;
	StateInitData.NiagaraCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Particle.Action.Move");
	StateInitData.SoundCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Sound.Action.Move");
	StateInitData.WaitTag = FGameplayTag(); //FGameplayTag::RequestGameplayTag("State.Action.Move");
	bIsUseWaitTag = false;
	SetAssetTags(StateInitData.MonsterAssetTags);
}

void UGA_MonsterState_Return::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);
}

void UGA_MonsterState_Return::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ABaseMonster* Monster = Cast<ABaseMonster>(GetOwningActorFromActorInfo());
	AAIController* AIC = Cast<AAIController>(Monster->GetController());
	FVector TargetLocation = Monster->GetStartLocation();

	if (IsValid(AIC))
	{
		AIC->ReceiveMoveCompleted.RemoveAll(this);
		AIC->ReceiveMoveCompleted.AddDynamic(this, &UGA_MonsterState_Return::OnMoveFinished);
		AIC->MoveToLocation(TargetLocation, 10.f, false);
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UGA_MonsterState_Return::OnMoveFinished(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
	ABaseMonster* Monster = Cast<ABaseMonster>(GetOwningActorFromActorInfo());
	if (IsValid(Monster))
	{
		Monster->SetbIsCombat(false);
		Monster->SetAttackUtility(1.f);
		Monster->SetActorRotation(Monster->GetStartRotator());
		Monster->SendStateTreeEvent(FGameplayTag::RequestGameplayTag("Event.Action.Return"));
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_MonsterState_Return::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ABaseMonster* Monster = Cast<ABaseMonster>(GetOwningActorFromActorInfo()))
	{
		if (AAIController* AIC = Cast<AAIController>(Monster->GetController()))
		{
			AIC->ReceiveMoveCompleted.RemoveAll(this);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_MonsterState_Return::OnMontageCompleted()
{
}

void UGA_MonsterState_Return::OnMontageBlendIn()
{
}

void UGA_MonsterState_Return::OnMontageBlendOut()
{
}

void UGA_MonsterState_Return::OnMontageInterrupt()
{
}

void UGA_MonsterState_Return::OnMontageCancel()
{
}

void UGA_MonsterState_Return::OnTagRemoved()
{
}
