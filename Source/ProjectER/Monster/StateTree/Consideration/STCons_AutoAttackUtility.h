#pragma once

#include "CoreMinimal.h"
#include "StateTreeConsiderationBase.h"
#include "StateTreeExecutionContext.h"
#include "STCons_AutoAttackUtility.generated.h"

USTRUCT()
struct FAutoAttackUtilityData
{
	GENERATED_BODY()

};

USTRUCT()
struct PROJECTER_API FSTCons_AutoAttackUtility : public FStateTreeConsiderationCommonBase
{
	GENERATED_BODY()
public:
	FSTCons_AutoAttackUtility();

	using FInstanceDataType = FAutoAttackUtilityData;

	virtual bool Link(FStateTreeLinker& Linker) override;

	virtual const UStruct* GetInstanceDataType() const override;

	virtual float GetScore(FStateTreeExecutionContext& Context) const override;

	TStateTreeExternalDataHandle<AActor> ActorHandle;
};
