#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "STT_SetCollisionProfile.generated.h"

USTRUCT()
struct FSetCollisionProfileData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "ProfileName")
	FCollisionProfileName StartProfileName;

	UPROPERTY(EditAnywhere, Category = "ProfileName")
	FCollisionProfileName EndProfileName;
};

USTRUCT()
struct PROJECTER_API FSTT_SetCollisionProfile : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()
public:
	FSTT_SetCollisionProfile();

	using FInstanceDataType = FSetCollisionProfileData;

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
