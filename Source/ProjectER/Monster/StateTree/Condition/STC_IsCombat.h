// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "STC_IsCombat.generated.h"

USTRUCT()
struct FIsCombatData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Invert")
	bool Invert = false;
};

USTRUCT()
struct PROJECTER_API FSTC_IsCombat : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()
public:
	FSTC_IsCombat();

	using FInstanceDataType = FIsCombatData;

	virtual bool Link(FStateTreeLinker& Linker) override;

	virtual const UStruct* GetInstanceDataType() const override;

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

	TStateTreeExternalDataHandle<AActor> ActorHandle;
};
