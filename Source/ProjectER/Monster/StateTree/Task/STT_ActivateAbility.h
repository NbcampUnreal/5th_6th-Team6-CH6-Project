#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "STT_ActivateAbility.generated.h"

USTRUCT()
struct FActivateAbilityData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Tag")
	FGameplayTag AbilityTag;
};

USTRUCT()
struct PROJECTER_API FSTT_ActivateAbility : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FSTT_ActivateAbility();

	using FInstanceDataType = FActivateAbilityData;

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
