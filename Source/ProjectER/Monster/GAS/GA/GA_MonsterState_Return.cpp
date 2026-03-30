#include "Monster/GAS/GA/GA_MonsterState_Return.h"
#include "Monster/BaseMonster.h"
#include "AbilitySystemComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

UGA_MonsterState_Return::UGA_MonsterState_Return()
{
	StateInitData.MonsterAssetTags = FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Ability.Action.Return"));
	StateInitData.MontageType = EMonsterMontageType::Move;
	StateInitData.NiagaraCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Particle.Action.Move");
	StateInitData.SoundCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Sound.Action.Move");
	StateInitData.WaitTag = FGameplayTag::RequestGameplayTag("State.Action.Move");
	bIsUseWaitTag = true;
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
	if (IsValid(Monster) == false || IsValid(Monster->MonsterData) == false)
	{
		//UE_LOG(LogTemp, Warning, TEXT("UGA_MonsterState_Return::ActivateAbility : Not Monster"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AAIController* AIC = Cast<AAIController>(Monster->GetController());
	FVector TargetLocation = Monster->GetStartLocation();

	if (IsValid(AIC) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGA_MonsterState_Return::ActivateAbility : Not AIC"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AIC->ReceiveMoveCompleted.RemoveAll(this);
	
	EPathFollowingRequestResult::Type ReqResult = AIC->MoveToLocation(TargetLocation, 10.f, false);
	if (ReqResult == EPathFollowingRequestResult::Failed)
	{
		ReqResult = AIC->MoveToLocation(TargetLocation, 100.f, false);
	}

	if (ReqResult != EPathFollowingRequestResult::Failed && ReqResult != EPathFollowingRequestResult::AlreadyAtGoal)
	{
		MoveRequestID = AIC->GetCurrentMoveRequestID();
		AIC->ReceiveMoveCompleted.AddDynamic(this, &UGA_MonsterState_Return::OnMoveFinished);
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
}

void UGA_MonsterState_Return::OnMoveFinished(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
	// 자신이 발송한 요청이 아니면 무시
	if (RequestID != MoveRequestID)
	{
		return;
	}
	
	ABaseMonster* Monster = Cast<ABaseMonster>(GetOwningActorFromActorInfo());
	if (IsValid(Monster))
	{
		Monster->SetIsFirstAttack(false);
		Monster->SetAttackCount(0);
		Monster->SetbIsCombat(false);
		Monster->SetActorRotation(Monster->GetStartRotator());
		Monster->SendStateTreeEvent(FGameplayTag::RequestGameplayTag("Event.Action.Return"));
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_MonsterState_Return::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	ABaseMonster* Monster = Cast<ABaseMonster>(GetOwningActorFromActorInfo());
	if (IsValid(Monster))
	{
		if (AAIController* AIC = Cast<AAIController>(Monster->GetController()))
		{
			AIC->ReceiveMoveCompleted.RemoveAll(this);
		}
	}
}

