#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "STT_ActivateTargetSkill.generated.h"

USTRUCT()
struct FSTT_ActivateTargetSkillData
{
	GENERATED_BODY()


};

USTRUCT()
struct PROJECTER_API FSTT_ActivateTargetSkill : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FSTT_ActivateTargetSkill();

	using FInstanceDataType = FSTT_ActivateTargetSkillData;

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
