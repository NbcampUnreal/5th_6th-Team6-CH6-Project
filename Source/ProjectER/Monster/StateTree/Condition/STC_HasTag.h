#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "STC_HasTag.generated.h"

USTRUCT()
struct FHasTagData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Tag")
	FGameplayTag CheckTag = FGameplayTag::EmptyTag;

	UPROPERTY(EditAnywhere, Category = "Invert")
	bool Invert = false;
};


USTRUCT()
struct PROJECTER_API FSTC_HasTag : public FStateTreeConditionBase
{
	GENERATED_BODY()

	FSTC_HasTag();

	using FInstanceDataType = FHasTagData;
	
	virtual bool Link(FStateTreeLinker& Linker) override;

	virtual const UStruct* GetInstanceDataType() const override;

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

	TStateTreeExternalDataHandle<AActor> ActorHandle;
};
