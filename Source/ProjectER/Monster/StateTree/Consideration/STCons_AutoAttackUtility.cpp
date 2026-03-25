#include "Monster/StateTree/Consideration/STCons_AutoAttackUtility.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"

#include "Monster/BaseMonster.h"

FSTCons_AutoAttackUtility::FSTCons_AutoAttackUtility()
{

}

bool FSTCons_AutoAttackUtility::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(ActorHandle);
	return true;
}

const UStruct* FSTCons_AutoAttackUtility::GetInstanceDataType() const
{
	return FInstanceDataType::StaticStruct();
}

float FSTCons_AutoAttackUtility::GetScore(FStateTreeExecutionContext& Context) const
{
	AActor& Actor = Context.GetExternalData(ActorHandle);
	if (!IsValid(&Actor))
	{
		return -1.f;
	}

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	ABaseMonster* Monster = Cast<ABaseMonster>(&Actor);

	if (!IsValid(Monster))
	{
		return -1.f;
	}

	if (Monster->GetIsFirstAttack() == false)
	{
		return 1.f;
	}

	return 0.f;
}
