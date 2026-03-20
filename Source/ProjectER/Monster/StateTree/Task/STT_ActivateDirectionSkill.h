#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "STT_ActivateDirectionSkill.generated.h"

USTRUCT()
struct FActivateDirectionSkillData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Tag")
	FGameplayTag AbilityTag;

	UPROPERTY(EditAnywhere, Category = "Tag")
	FGameplayTag EventTag;
};

USTRUCT()
struct PROJECTER_API FSTT_ActivateDirectionSkill : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()
public:
	FSTT_ActivateDirectionSkill();

	using FInstanceDataType = FActivateDirectionSkillData;

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
