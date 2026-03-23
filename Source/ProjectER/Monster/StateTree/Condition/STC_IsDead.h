#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "STC_IsDead.generated.h"

USTRUCT()
struct FIsDeadData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Invert")
	bool Invert = false;
};

USTRUCT()
struct PROJECTER_API FSTC_IsDead : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()
public:
	FSTC_IsDead();

	using FInstanceDataType = FIsDeadData;

	virtual bool Link(FStateTreeLinker& Linker) override;

	virtual const UStruct* GetInstanceDataType() const override;

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

	TStateTreeExternalDataHandle<AActor> ActorHandle;
};
