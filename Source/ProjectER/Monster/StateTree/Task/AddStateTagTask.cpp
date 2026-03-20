#include "Monster/StateTree/Task/AddStateTagTask.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"

FAddStateTagTask::FAddStateTagTask()
{
	bShouldCallTick = false;
}

bool FAddStateTagTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(ActorHandle);
	return true;
}

const UStruct* FAddStateTagTask::GetInstanceDataType() const
{
	return FAddStateTagData::StaticStruct();
}

EStateTreeRunStatus FAddStateTagTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	Context.GetExternalData(ActorHandle);


	//const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	//AActor* Actor = InstanceData.Actor;
	//if (!Actor)
	//{
	//	return EStateTreeRunStatus::Failed;
	//}

	//// 예: GameplayTag를 Actor에 추가
	//if (IGameplayTagAssetInterface* TagInterface = Cast<IGameplayTagAssetInterface>(Actor))
	//{
	//	// 실제로는 Add 함수 없음 → 보통 AbilitySystem 사용
	//}

	return EStateTreeRunStatus::Succeeded;
}

void FAddStateTagTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	//const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	//AActor* Actor = InstanceData.Actor;
	//if (!Actor) return;

	//UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor);

	//if (ASC)
	//{
	//	ASC->RemoveLooseGameplayTag(InstanceData.StateTag);
	//}
}


