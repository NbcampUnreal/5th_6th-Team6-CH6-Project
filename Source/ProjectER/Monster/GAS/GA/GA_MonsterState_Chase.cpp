#include "Monster/GAS/GA/GA_MonsterState_Chase.h"
#include "Monster/BaseMonster.h"
#include "AbilitySystemComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Monster/GAS/AttributeSet/BaseMonsterAttributeSet.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"



UGA_MonsterState_Chase::UGA_MonsterState_Chase()
{
	StateInitData.MonsterAssetTags = FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Ability.Action.Chase"));
	StateInitData.MontageType = EMonsterMontageType::Move;
	StateInitData.NiagaraCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Particle.Action.Move");
	StateInitData.SoundCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Sound.Action.Move");
	StateInitData.WaitTag = FGameplayTag::RequestGameplayTag("State.Action.Move");
	bIsUseWaitTag = true;
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
	if (IsValid(Monster) == false || IsValid(Monster->MonsterData) == false)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AAIController* AIC = Cast<AAIController>(Monster->GetController());
	AActor* TargetActor = Monster->GetTargetPlayer();

	if (IsValid(AIC) == false || IsValid(TargetActor) == false)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Monster->SetbIsCombat(true);

	float AttackRange = 0.0f;
	UBaseMonsterAttributeSet* AS = Monster->GetAttributeSet();
	if (IsValid(AS))
	{
		AttackRange = AS->GetAttackRange();
	}
	float CapsuleRadius = Monster->GetCapsuleComponent()->GetScaledCapsuleRadius();
	float AcceptanceRadius = FMath::Max(0.0f, AttackRange - (CapsuleRadius + 10));

	AIC->ReceiveMoveCompleted.RemoveAll(this);
	
	EPathFollowingRequestResult::Type ReqResult = AIC->MoveToActor(TargetActor, AcceptanceRadius, false);
	if (ReqResult != EPathFollowingRequestResult::Failed && ReqResult != EPathFollowingRequestResult::AlreadyAtGoal)
	{
		MoveRequestID = AIC->GetCurrentMoveRequestID();
		AIC->ReceiveMoveCompleted.AddDynamic(this, &UGA_MonsterState_Chase::OnMoveFinished);
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
}


void UGA_MonsterState_Chase::OnMoveFinished(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
	if (RequestID != MoveRequestID)
	{
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_MonsterState_Chase::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	ABaseMonster* Monster = Cast<ABaseMonster>(GetOwningActorFromActorInfo());
	if (IsValid(Monster))
	{
		if (AAIController* AIC = Cast<AAIController>(Monster->GetController()))
		{
			AIC->ReceiveMoveCompleted.RemoveAll(this);
		}

		if (UAnimInstance* AnimInstance = Monster->GetMesh()->GetAnimInstance())
		{
			AnimInstance->StopAllMontages(0.f);
		}
	}

	UBaseMonsterAttributeSet* AS = Monster->GetAttributeSet();
	if(IsValid(AS))
	{
		Monster->SendAttackRangeEvent(AS->GetAttackRange());
	}

}
