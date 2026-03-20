#include "Monster/StateTree/Task/STT_ActivateTargetSkill.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"

FSTT_ActivateTargetSkill::FSTT_ActivateTargetSkill()
{
	bShouldCallTick = false;
}

bool FSTT_ActivateTargetSkill::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(ActorHandle);
	return false;
}

const UStruct* FSTT_ActivateTargetSkill::GetInstanceDataType() const
{
	return FInstanceDataType::StaticStruct();
}

EStateTreeRunStatus FSTT_ActivateTargetSkill::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{

	return EStateTreeRunStatus::Failed;
}

void FSTT_ActivateTargetSkill::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
}
