#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "STT_TransitionToCombat.generated.h"

USTRUCT()
struct FTransitionToCombatData
{
	GENERATED_BODY()


};

USTRUCT()
struct PROJECTER_API FSTT_TransitionToCombat : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()
public:
	FSTT_TransitionToCombat();

	using FInstanceDataType = FTransitionToCombatData;

	virtual bool Link(FStateTreeLinker& Linker) override;

	virtual const UStruct* GetInstanceDataType() const override;
	

	virtual EStateTreeRunStatus EnterState(
		FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition
	) const override;

	virtual void ExitState(
		FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition
	) const override;

	TStateTreeExternalDataHandle<AActor> ActorHandle;
};
