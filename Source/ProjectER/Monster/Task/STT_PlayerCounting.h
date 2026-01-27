#pragma once

#include "CoreMinimal.h"
#include "Blueprint/StateTreeTaskBlueprintBase.h"
#include "STT_PlayerCounting.generated.h"

class USphereComponent;

USTRUCT()
struct FCheckingSplineDistanceData
{
    GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Task", meta = (AllowprivateAccess = "true"))
	float FindRadius;

	USphereComponent* Sphere = nullptr;

	UPROPERTY(EditAnywhere, Category = "Task", meta = (AllowprivateAccess = "true"))
	bool Isbool;
};

UCLASS()
class PROJECTER_API USTT_PlayerCounting : public UStateTreeTaskBlueprintBase
{
	GENERATED_BODY()


	UStruct* GetInstanceDataType() const { return FCheckingSplineDistanceData::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(
		FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition
	) override;

	virtual EStateTreeRunStatus Tick(
		FStateTreeExecutionContext& Context,
		const float DeltaTime
	) override;

	virtual void ExitState(
		FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition
	) override;


	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UPROPERTY(EditAnywhere, Category = "Task", meta = (AllowprivateAccess = "true"))
	float FindRadius;

	USphereComponent* Sphere = nullptr;

	UPROPERTY(EditAnywhere, Category = "Task", meta = (AllowprivateAccess = "true"))
	bool Isbool;

};
