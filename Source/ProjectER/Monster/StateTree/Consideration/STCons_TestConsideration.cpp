#include "Monster/StateTree/Consideration/STCons_TestConsideration.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"

FSTCons_TestConsideration::FSTCons_TestConsideration()
{
}

bool FSTCons_TestConsideration::Link(FStateTreeLinker& Linker)
{
	//Linker.LinkExternalData(ActorHandle);
	return true;
}

const UStruct* FSTCons_TestConsideration::GetInstanceDataType() const
{
	return FInstanceDataType::StaticStruct();
}

float FSTCons_TestConsideration::GetScore(FStateTreeExecutionContext& Context) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	
	return InstanceData.Input;
}
