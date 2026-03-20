#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "STT_ActivateGroundSkill.generated.h"

USTRUCT()
struct FActivateGroundSkillData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Tag")
	FGameplayTag AbilityTag;

	UPROPERTY(EditAnywhere, Category = "Tag")
	FGameplayTag EventTag;
};

USTRUCT()
struct PROJECTER_API FSTT_ActivateGroundSkill : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()
public:
	FSTT_ActivateGroundSkill();

	using FInstanceDataType = FActivateGroundSkillData;

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
