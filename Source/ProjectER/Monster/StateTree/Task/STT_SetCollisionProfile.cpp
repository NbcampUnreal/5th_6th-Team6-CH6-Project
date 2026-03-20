#include "Monster/StateTree/Task/STT_SetCollisionProfile.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"
#include "Monster/BaseMonster.h"
#include "Components/CapsuleComponent.h"

FSTT_SetCollisionProfile::FSTT_SetCollisionProfile()
{
	bShouldCallTick = false;
}

bool FSTT_SetCollisionProfile::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(ActorHandle);
	return true;
}

const UStruct* FSTT_SetCollisionProfile::GetInstanceDataType() const
{
	return FInstanceDataType::StaticStruct();
}

EStateTreeRunStatus FSTT_SetCollisionProfile::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	AActor& Actor = Context.GetExternalData(ActorHandle);
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (ABaseMonster* Monster = Cast<ABaseMonster>(&Actor))
	{
		Monster->Multicast_SetCollisionProfileName(InstanceData.StartProfileName.Name);

		return EStateTreeRunStatus::Running;
	}

	return EStateTreeRunStatus::Failed;
}

void FSTT_SetCollisionProfile::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	AActor& Actor = Context.GetExternalData(ActorHandle);
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (ABaseMonster* Monster = Cast<ABaseMonster>(&Actor))
	{
		Monster->Multicast_SetCollisionProfileName(InstanceData.EndProfileName.Name);
	}
}
