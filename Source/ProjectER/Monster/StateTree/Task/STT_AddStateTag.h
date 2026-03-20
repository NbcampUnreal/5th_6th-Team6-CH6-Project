// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "ActiveGameplayEffectHandle.h"
#include "STT_AddStateTag.generated.h"

class UGameplayEffect;

USTRUCT()
struct FAddStateTagData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Tag")
	TSubclassOf<UGameplayEffect> TagEffect;

	UPROPERTY(EditAnywhere, Category = "Tag")
	FGameplayTag StateTag;

	UPROPERTY()
	FActiveGameplayEffectHandle ActiveEffectHandle;
};


USTRUCT()
struct PROJECTER_API FSTT_AddStateTag : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()
public:
	FSTT_AddStateTag();
	
	using FInstanceDataType = FAddStateTagData;

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
