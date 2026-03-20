// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "GameFramework/Pawn.h"
#include "AddStateTagTask.generated.h"


USTRUCT()
struct FAddStateTagData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Tag")
	FGameplayTag StateTag;
};


USTRUCT()
struct PROJECTER_API FAddStateTagTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()
public:
	FAddStateTagTask();
	
	//using FAddStateTagDataType = FAddStateTagTask;

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


	TStateTreeExternalDataHandle <APawn> ActorHandle;
};
