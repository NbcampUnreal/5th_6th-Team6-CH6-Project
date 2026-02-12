// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameAbility/MouseClickSkill.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "AbilitySystemComponent.h"

void UMouseClickSkill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	PrepareDataSetDelegate(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (IsLocallyControlled())
	{
		SendLocationData(GetMouseLocation());
	}
	else {
		//DoNoting
	}
}

void UMouseClickSkill::SendLocationData(FVector TargetLocation)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	FGameplayAbilityTargetDataHandle DataHandle;
	FGameplayAbilityTargetData_LocationInfo* LocData = new FGameplayAbilityTargetData_LocationInfo();
	LocData->TargetLocation.LiteralTransform = FTransform(TargetLocation);
	DataHandle.Add(LocData);

	FScopedPredictionWindow ScopedPrediction(ASC, CurrentActorInfo->IsLocallyControlled());
	ASC->CallServerSetReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey(), DataHandle, FGameplayTag(), ASC->ScopedPredictionKey);
	//ASC->CallReplicatedTargetDataDelegatesIfSet(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
}

bool UMouseClickSkill::IsInRange(const FVector& Location)
{
	return false;
}

void UMouseClickSkill::RotateToLocation(const FVector& Location)
{
}

void UMouseClickSkill::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ActivationTag)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	check(ASC);
	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo)) return;

	FVector FinalLocation = DataHandle.Get(0)->GetEndPoint();

	// 장판 소환!!
	//SpawnSummonArea(FinalLocation);

	// 마무리 작업
	ASC->AbilityTargetDataSetDelegate(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey()).Remove(TargetDataDelegateHandle);
	ASC->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
}

void UMouseClickSkill::ExecuteSkill()
{

}


void UMouseClickSkill::PrepareDataSetDelegate(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	check(ASC);

	TargetDataDelegateHandle = ASC->AbilityTargetDataSetDelegate(Handle, ActivationInfo.GetActivationPredictionKey()).AddUObject(this, &UMouseClickSkill::OnTargetDataReady);
	ASC->CallReplicatedTargetDataDelegatesIfSet(Handle, ActivationInfo.GetActivationPredictionKey());
}

FVector UMouseClickSkill::GetMouseLocation() const
{
	APlayerController* PC = GetActorInfo().PlayerController.Get();
	if (PC)
	{
		FHitResult Hit;
		if (PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
		{
			return Hit.Location;
		}
	}
	return FVector::ZeroVector;
}
