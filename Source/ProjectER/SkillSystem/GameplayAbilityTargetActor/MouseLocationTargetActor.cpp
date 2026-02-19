// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplayAbilityTargetActor/MouseLocationTargetActor.h"
#include "SkillSystem/GameAbility/MouseClickSkill.h"
#include "Abilities/GameplayAbilityTargetTypes.h"

namespace
{
	FGameplayAbilityTargetDataHandle MakeLocationTargetData(const FVector& Location)
	{
		FGameplayAbilityTargetDataHandle DataHandle;
		FGameplayAbilityTargetData_LocationInfo* LocData = new FGameplayAbilityTargetData_LocationInfo();
		LocData->TargetLocation.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
		LocData->TargetLocation.LiteralTransform = FTransform(Location);
		DataHandle.Add(LocData);
		return DataHandle;
	}
}


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

	TargetDataReadyDelegate.Broadcast(MakeLocationTargetData(MouseLocation));
	return true;
}

bool AMouseLocationTargetActor::SubmitExternalLocation(const FVector& InLocation)
{
	UMouseClickSkill* MouseClickSkill = Cast<UMouseClickSkill>(OwningAbility);
	if (!IsValid(MouseClickSkill)) return false;
	if (!MouseClickSkill->IsTargetLocationInRange(InLocation)) return false;

	TargetDataReadyDelegate.Broadcast(MakeLocationTargetData(InLocation));
	return true;
}