#include "Monster/StateTree/Task/STT_ActivateAbility.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

FSTT_ActivateAbility::FSTT_ActivateAbility()
{
	bShouldCallTick = false;
}

bool FSTT_ActivateAbility::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(ActorHandle);
	return true;
}

const UStruct* FSTT_ActivateAbility::GetInstanceDataType() const
{
	return FInstanceDataType::StaticStruct();
}

EStateTreeRunStatus FSTT_ActivateAbility::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	AActor& Actor = Context.GetExternalData(ActorHandle);
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(&Actor);

	FInstanceDataType& InstnaceData = Context.GetInstanceData(*this);
	FGameplayTagContainer Container = InstnaceData.AbilityTag.GetSingleTagContainer();

	if (ASC->TryActivateAbilitiesByTag(Container))
	{
		return EStateTreeRunStatus::Running;
	}

	return EStateTreeRunStatus::Failed;
}

void FSTT_ActivateAbility::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{

}
