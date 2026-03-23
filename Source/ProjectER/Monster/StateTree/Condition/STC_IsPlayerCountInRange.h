#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "STC_IsPlayerCountInRange.generated.h"

UENUM()
enum class EPlayerCountOperator : uint8
{
	Equal,
	NotEqual,
	Greater,
	GreaterOrEqual,
	Less,
	LessOrEqual,
	None
};

USTRUCT()
struct FIsPlayerCountInRangeData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Condition")
	EPlayerCountOperator Operator = EPlayerCountOperator::None;

	UPROPERTY(EditAnywhere, Category = "Condition")
	int32 CheckCount = 0;
};

USTRUCT(meta = (DisplayName = "Is Player Count In Range"))
struct PROJECTER_API FSTC_IsPlayerCountInRange : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()
public:
	FSTC_IsPlayerCountInRange();

	using FInstanceDataType = FIsPlayerCountInRangeData;

	virtual bool Link(FStateTreeLinker& Linker) override;

	virtual const UStruct* GetInstanceDataType() const override;

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

	TStateTreeExternalDataHandle<AActor> ActorHandle;
};
