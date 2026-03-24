#pragma once

#include "CoreMinimal.h"
#include "StateTreeConsiderationBase.h"
#include "STCons_TestConsideration.generated.h"

USTRUCT()
struct FSTCons_TestConsiderationData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Input")
	float Input = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Input")
	FRuntimeFloatCurve ResponseCurve;

	UPROPERTY(EditAnywhere, Category = "Input")
	float Weight;
};

USTRUCT()
struct PROJECTER_API FSTCons_TestConsideration : public FStateTreeConsiderationCommonBase
{
	GENERATED_BODY()
public:
	FSTCons_TestConsideration();

	using FInstanceDataType = FSTCons_TestConsiderationData;

	virtual bool Link(FStateTreeLinker& Linker) override;

	virtual const UStruct* GetInstanceDataType() const override;

	virtual float GetScore(FStateTreeExecutionContext& Context) const override;

	//TStateTreeExternalDataHandle<AActor> ActorHandle;
};
