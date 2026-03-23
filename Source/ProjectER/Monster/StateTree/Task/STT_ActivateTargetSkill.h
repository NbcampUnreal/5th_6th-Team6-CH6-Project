#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "STT_ActivateTargetSkill.generated.h"

USTRUCT()
struct FActivateTargetSkillData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Tag")
	FGameplayTag AbilityTag;

	UPROPERTY(EditAnywhere, Category = "Tag")
	FGameplayTag EventTag;
};

USTRUCT()
struct PROJECTER_API FSTT_ActivateTargetSkill : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FSTT_ActivateTargetSkill();

	using FInstanceDataType = FActivateTargetSkillData;

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
