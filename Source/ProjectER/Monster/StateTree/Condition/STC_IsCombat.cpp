#include "Monster/StateTree/Condition/STC_IsCombat.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"
#include "Monster/BaseMonster.h"

FSTC_IsCombat::FSTC_IsCombat()
{
}

bool FSTC_IsCombat::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(ActorHandle);
	return true;
}

const UStruct* FSTC_IsCombat::GetInstanceDataType() const
{
	return FInstanceDataType::StaticStruct();
}

bool FSTC_IsCombat::TestCondition(FStateTreeExecutionContext& Context) const
{
	AActor& Actor = Context.GetExternalData(ActorHandle);
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (ABaseMonster* Monster = Cast<ABaseMonster>(&Actor))
	{
		bool IsCombat = Monster->GetbIsCombat();
		return IsCombat != InstanceData.Invert;
	}

	return false;
}
