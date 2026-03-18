#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlatformUpdatorActor.generated.h"

UCLASS()
class PROJECTER_API APlatformUpdatorActor : public AActor
{
	GENERATED_BODY()

public:

	APlatformUpdatorActor();

	/* ---------- Config ---------- */

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MPC")
	TObjectPtr<UMaterialParameterCollection> TargetMPC = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MPC")
	bool bUseSelfAsPivot = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MPC", meta=(EditCondition="!bUseSelfAsPivot"))
	FVector ManualPivot = FVector::ZeroVector;

	/* ---------- Runtime ---------- */

	UFUNCTION(BlueprintCallable, Category="MPC")
	void PushTransformToMPC();

	UFUNCTION(BlueprintCallable, Category="MPC")
	void SetInitialLocation();

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:

	FVector InitialLocation;
	FQuat   InitialRotation;
};