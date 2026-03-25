#include "Monster/StateTree/Consideration/STCons_SkillUtility.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"

#include "Monster/BaseMonster.h"

FSTCons_SkillUtility::FSTCons_SkillUtility()
{
}

bool FSTCons_SkillUtility::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(ActorHandle);
	return true;
}

const UStruct* FSTCons_SkillUtility::GetInstanceDataType() const
{
	return FInstanceDataType::StaticStruct();
}

float FSTCons_SkillUtility::GetScore(FStateTreeExecutionContext& Context) const
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

	return 0.5f;
}
