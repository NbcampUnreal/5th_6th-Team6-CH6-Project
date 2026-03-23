#include "Monster/StateTree/Task/STT_TransitionToCombat.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"

#include "Monster/BaseMonster.h"

FSTT_TransitionToCombat::FSTT_TransitionToCombat()
{
	bShouldCallTick = false;
}

bool FSTT_TransitionToCombat::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(ActorHandle);
	return true;
}

const UStruct* FSTT_TransitionToCombat::GetInstanceDataType() const
{
	return FInstanceDataType::StaticStruct();
}

EStateTreeRunStatus FSTT_TransitionToCombat::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	AActor& Actor = Context.GetExternalData(ActorHandle);
	ABaseMonster* Monster = Cast<ABaseMonster>(&Actor);
	if (IsValid(Monster))
	{
		Monster->OffCCChanged();
		return EStateTreeRunStatus::Running;
	}
		
	return EStateTreeRunStatus::Failed;
}

void FSTT_TransitionToCombat::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
}
