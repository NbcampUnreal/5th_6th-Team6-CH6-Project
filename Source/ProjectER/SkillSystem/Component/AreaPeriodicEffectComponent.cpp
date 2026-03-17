// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/Component/AreaPeriodicEffectComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"

UAreaPeriodicEffectComponent::UAreaPeriodicEffectComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bAutoActivate = true;
}

void UAreaPeriodicEffectComponent::SetupPeriodicEffect(float InPeriod, bool bInApplyImmediately)
{
	Period = InPeriod;
	bApplyImmediately = bInApplyImmediately;
}

void UAreaPeriodicEffectComponent::StartPeriodicTrigger()
{
	if (Period > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			PeriodicTimerHandle, 
			this, 
			&UAreaPeriodicEffectComponent::OnTimerTrigger, 
			Period, 
			true, 
			bApplyImmediately ? 0.f : Period
		);
	}
}

void UAreaPeriodicEffectComponent::StopPeriodicTrigger()
{
	if (GetWorld()->GetTimerManager().IsTimerActive(PeriodicTimerHandle))
	{
		GetWorld()->GetTimerManager().ClearTimer(PeriodicTimerHandle);
	}
}

void UAreaPeriodicEffectComponent::AddTarget(AActor* InTarget)
{
	if (IsValid(InTarget))
	{
		CurrentTargets.Add(InTarget);
	}
}

void UAreaPeriodicEffectComponent::RemoveTarget(AActor* InTarget)
{
	if (IsValid(InTarget))
	{
		CurrentTargets.Remove(InTarget);
	}
}

void UAreaPeriodicEffectComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopPeriodicTrigger();
	Super::EndPlay(EndPlayReason);
}

void UAreaPeriodicEffectComponent::OnTimerTrigger()
{
	if (CurrentTargets.Num() <= 0)
	{
		return;
	}

	// 유효하지 않은 액터 제거 및 배열 변환
	TArray<AActor*> ValidTargets;
	for (auto It = CurrentTargets.CreateIterator(); It; ++It)
	{
		AActor* Actor = It->Get();
		if (IsValid(Actor))
		{
			ValidTargets.Add(Actor);
		}
		else
		{
			It.RemoveCurrent();
		}
	}

	if (ValidTargets.Num() > 0)
	{
		OnAreaPeriodicTrigger.Broadcast(ValidTargets);
	}
}
