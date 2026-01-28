// Fill out your copyright notice in the Description page of Project Settings.
#include "Monster/Task/STT_PlayerCounting.h"

#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"
#include "Monster/BaseMonster.h"

bool FSTT_PlayerCounting::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(MonsterHandle);
	return true;
}

EStateTreeRunStatus FSTT_PlayerCounting::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	ABaseMonster* Monster = &Context.GetExternalData(MonsterHandle);
	if (IsValid(Monster) == false)
	{
		return EStateTreeRunStatus::Failed;
	}

	return EStateTreeRunStatus();
}

EStateTreeRunStatus FSTT_PlayerCounting::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	return EStateTreeRunStatus();
}

void FSTT_PlayerCounting::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
}
