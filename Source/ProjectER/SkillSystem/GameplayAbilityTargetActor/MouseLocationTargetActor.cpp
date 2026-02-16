// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplayAbilityTargetActor/MouseLocationTargetActor.h"
#include "SkillSystem/GameAbility/MouseClickSkill.h"
#include "Abilities/GameplayAbilityTargetTypes.h"

void AMouseLocationTargetActor::ConfirmTargetingAndContinue()
{
	if (TryConfirmMouseLocation() == false)
	{
		FGameplayAbilityTargetDataHandle CancelHandle;
		CanceledDelegate.Broadcast(CancelHandle);
	}
}

bool AMouseLocationTargetActor::TryConfirmMouseLocation()
{
	UMouseClickSkill* MouseClickSkill = Cast<UMouseClickSkill>(OwningAbility);
	if (!IsValid(MouseClickSkill)) return false;

	FVector MouseLocation = FVector::ZeroVector;
	if (!MouseClickSkill->TryGetMouseLocationInRange(MouseLocation)) return false;

	FGameplayAbilityTargetDataHandle DataHandle;
	FGameplayAbilityTargetData_LocationInfo* LocData = new FGameplayAbilityTargetData_LocationInfo();
	LocData->TargetLocation.LiteralTransform = FTransform(MouseLocation);
	DataHandle.Add(LocData);

	TargetDataReadyDelegate.Broadcast(DataHandle);
	return true;
}
