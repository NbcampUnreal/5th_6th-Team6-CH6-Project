#include "Monster/StateTree/Condition/STC_HasTag.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

FSTC_HasTag::FSTC_HasTag()
{
	
}

bool FSTC_HasTag::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(ActorHandle);
	return true;
}

const UStruct* FSTC_HasTag::GetInstanceDataType() const
{
	return FInstanceDataType::StaticStruct();
}

bool FSTC_HasTag::TestCondition(FStateTreeExecutionContext& Context) const
{
	AActor& Actor = Context.GetExternalData(ActorHandle);
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	UAbilitySystemComponent* ASC = 
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(&Actor);

	return ASC->HasMatchingGameplayTag(InstanceData.CheckTag) != InstanceData.Invert;
}
