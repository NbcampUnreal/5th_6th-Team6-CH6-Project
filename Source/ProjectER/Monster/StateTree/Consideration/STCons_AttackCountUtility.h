#pragma once

#include "CoreMinimal.h"
#include "StateTreeConsiderationBase.h"
#include "StateTreeExecutionContext.h"
#include "STCons_AttackCountUtility.generated.h"

USTRUCT()
struct FAttackCountUtilityData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter")
	uint8 AttackCountThreshold = 0;

};

USTRUCT()
struct PROJECTER_API FSTCons_AttackCountUtility : public FStateTreeConsiderationCommonBase
{
	GENERATED_BODY()
public:
	FSTCons_AttackCountUtility();

	using FInstanceDataType = FAttackCountUtilityData;

	virtual bool Link(FStateTreeLinker& Linker) override;

	virtual const UStruct* GetInstanceDataType() const override;

	virtual float GetScore(FStateTreeExecutionContext& Context) const override;

	TStateTreeExternalDataHandle<AActor> ActorHandle;
};
