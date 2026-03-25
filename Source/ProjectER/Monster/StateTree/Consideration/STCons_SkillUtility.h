#pragma once

#include "CoreMinimal.h"
#include "StateTreeConsiderationBase.h"
#include "StateTreeExecutionContext.h"
#include "STCons_SkillUtility.generated.h"

USTRUCT()
struct FSkillUtilityData
{
	GENERATED_BODY()

};

USTRUCT()
struct PROJECTER_API FSTCons_SkillUtility : public FStateTreeConsiderationCommonBase
{
	GENERATED_BODY()
public:
	FSTCons_SkillUtility();

	using FInstanceDataType = FSkillUtilityData;

	virtual bool Link(FStateTreeLinker& Linker) override;

	virtual const UStruct* GetInstanceDataType() const override;

	virtual float GetScore(FStateTreeExecutionContext& Context) const override;

	TStateTreeExternalDataHandle<AActor> ActorHandle;
};
