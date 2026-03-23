// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "STT_AttackRangeCheck.generated.h"

USTRUCT()
struct FAttackRangeCheckData
{
	GENERATED_BODY()
};


USTRUCT()
struct PROJECTER_API FSTT_AttackRangeCheck : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()
public:
	FSTT_AttackRangeCheck();

	using FInstanceDataType = FAttackRangeCheckData;

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
