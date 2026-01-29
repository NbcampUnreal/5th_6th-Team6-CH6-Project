// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "STT_PlayerCounting.generated.h"

class ABaseMonster;
class USphereComponent;

USTRUCT()
struct FSTT_PlayerCountingInstaceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Task", meta = (AllowprivateAccess = "true"))
	USphereComponent* Sphere;

	UPROPERTY(EditAnywhere, Category = "Task", meta = (AllowprivateAccess = "true"))
	float FindRadius;
};

USTRUCT()
struct FSTT_PlayerCounting : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTT_PlayerCountingInstaceData;

	virtual const UStruct* GetInstanceDataType() const override { return FSTT_PlayerCountingInstaceData::StaticStruct(); }
	virtual bool Link(FStateTreeLinker& Linker) override;

	virtual EStateTreeRunStatus EnterState(
		FStateTreeExecutionContext& Context, 
		const FStateTreeTransitionResult& Transition
	) const override;

	virtual EStateTreeRunStatus Tick(
		FStateTreeExecutionContext& Context,
		const float DeltaTime
	) const override;

	virtual void ExitState(
		FStateTreeExecutionContext& Context, 
		const FStateTreeTransitionResult& Transition
	) const override;

	TStateTreeExternalDataHandle<ABaseMonster> MonsterHandle;
};
