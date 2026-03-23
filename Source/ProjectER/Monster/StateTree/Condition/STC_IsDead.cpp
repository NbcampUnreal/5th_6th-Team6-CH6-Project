#include "Monster/StateTree/Condition/STC_IsDead.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"
#include "Monster/BaseMonster.h"

FSTC_IsDead::FSTC_IsDead()
{
}

bool FSTC_IsDead::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(ActorHandle);
	return true;
}

const UStruct* FSTC_IsDead::GetInstanceDataType() const
{
	return FInstanceDataType::StaticStruct();
}

bool FSTC_IsDead::TestCondition(FStateTreeExecutionContext& Context) const
{
	AActor& Actor = Context.GetExternalData(ActorHandle);
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (ABaseMonster* Monster = Cast<ABaseMonster>(&Actor))
	{
		bool IsCombat = Monster->GetbIsDead();
		return IsCombat != InstanceData.Invert;
	}

	return false;
}
