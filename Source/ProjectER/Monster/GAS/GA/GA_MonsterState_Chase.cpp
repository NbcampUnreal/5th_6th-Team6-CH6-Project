#include "Monster/GAS/GA/GA_MonsterState_Chase.h"
#include "Monster/BaseMonster.h"
#include "Monster/GAS/AttributeSet/BaseMonsterAttributeSet.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"


UGA_MonsterState_Chase::UGA_MonsterState_Chase()
{
	StateInitData.MonsterAssetTags = FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Ability.Action.Chase"));
	StateInitData.MontageType = EMonsterMontageType::Move;
	StateInitData.NiagaraCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Particle.Action.Move");
	StateInitData.SoundCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Sound.Action.Move");
	StateInitData.WaitTag = FGameplayTag::RequestGameplayTag("State.Action.Move");
	bIsUseWaitTag = false;
	SetAssetTags(StateInitData.MonsterAssetTags);
}

void UGA_MonsterState_Chase::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);
}

void UGA_MonsterState_Chase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ABaseMonster* Monster = Cast<ABaseMonster>(GetOwningActorFromActorInfo());
	AAIController* AIC = Cast<AAIController>(Monster->GetController());
	AActor* TargetActor = Monster->GetTargetPlayer();

	if (IsValid(AIC) && IsValid(TargetActor))
	{
		Monster->SetbIsCombat(true);

		float AttackRange = 0.0f;
		UBaseMonsterAttributeSet* AS = Monster->GetAttributeSet();
		if (IsValid(AS))
		{
			AttackRange = AS->GetAttackRange();
		}
		float CapsuleRadius = Monster->GetCapsuleComponent()->GetScaledCapsuleRadius();
		float AcceptanceRadius = FMath::Max(0.0f, AttackRange - CapsuleRadius);

		AIC->ReceiveMoveCompleted.RemoveAll(this);
		AIC->ReceiveMoveCompleted.AddDynamic(this, &UGA_MonsterState_Chase::OnMoveFinished);
		AIC->MoveToActor(TargetActor, AcceptanceRadius, false);
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}


void UGA_MonsterState_Chase::OnMoveFinished(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_MonsterState_Chase::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	ABaseMonster* Monster = Cast<ABaseMonster>(GetOwningActorFromActorInfo());
	if (IsValid(Monster))
	{
		if (AAIController* AIC = Cast<AAIController>(Monster->GetController()))
		{
			AIC->ReceiveMoveCompleted.RemoveAll(this);
		}
	}
	if (UBaseMonsterAttributeSet* AS = Monster->GetAttributeSet())
	{
		Monster->SendAttackRangeEvent(AS->GetAttackRange());
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_MonsterState_Chase::OnMontageCompleted()
{
}

void UGA_MonsterState_Chase::OnMontageBlendIn()
{
}

void UGA_MonsterState_Chase::OnMontageBlendOut()
{
}

void UGA_MonsterState_Chase::OnMontageInterrupt()
{
}

void UGA_MonsterState_Chase::OnMontageCancel()
{
}

void UGA_MonsterState_Chase::OnTagRemoved()
{
}
