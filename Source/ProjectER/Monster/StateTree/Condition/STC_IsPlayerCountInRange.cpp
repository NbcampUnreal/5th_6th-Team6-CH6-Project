#include "Monster/StateTree/Condition/STC_IsPlayerCountInRange.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"

#include "Monster/BaseMonster.h"
#include "Monster/MonsterRangeComponent.h"

FSTC_IsPlayerCountInRange::FSTC_IsPlayerCountInRange()
{
}

bool FSTC_IsPlayerCountInRange::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(ActorHandle);
	return true;
}

const UStruct* FSTC_IsPlayerCountInRange::GetInstanceDataType() const
{
	return FInstanceDataType::StaticStruct();
}

bool FSTC_IsPlayerCountInRange::TestCondition(FStateTreeExecutionContext& Context) const
{
	AActor& Actor = Context.GetExternalData(ActorHandle);
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (ABaseMonster* Monster = Cast<ABaseMonster>(&Actor))
	{
		uint8 CurrentCount = Monster->GetMonsterRangeComp()->GetPlayerCount();
		
		switch (InstanceData.Operator)
		{
		case EPlayerCountOperator::Equal:
			return CurrentCount == InstanceData.CheckCount;
		case EPlayerCountOperator::NotEqual:
			return CurrentCount != InstanceData.CheckCount;
		case EPlayerCountOperator::Greater:
			return CurrentCount > InstanceData.CheckCount;
		case EPlayerCountOperator::GreaterOrEqual:
			return CurrentCount >= InstanceData.CheckCount;
		case EPlayerCountOperator::Less:
			return CurrentCount < InstanceData.CheckCount;
		case EPlayerCountOperator::LessOrEqual:
			return CurrentCount <= InstanceData.CheckCount;
		default:
			return false;
		}
	}
	return false;
}
