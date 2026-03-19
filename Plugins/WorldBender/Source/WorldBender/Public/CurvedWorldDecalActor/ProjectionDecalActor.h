#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectionDecalActor.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UCLASS()
class WORLDBENDER_API AProjectionDecalActor : public AActor
{
	GENERATED_BODY()

public:
	AProjectionDecalActor();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	// Box mesh (projection volume)
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* BoxMesh;

	// Material with projection logic
	UPROPERTY(EditAnywhere, Category="Projection")
	UMaterialInterface* BaseMaterial;

	// Half size of projection box
	UPROPERTY(EditAnywhere, Category="Projection")
	FVector ProjectionExtent = FVector(100.f, 100.f, 100.f);


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projection")
	FName Param_ProjectionCenter=TEXT("ProjectionCenter");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projection")
	FName Param_ProjectionRight=TEXT("ProjectionRight");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projection")
	FName Param_ProjectionForward=TEXT("ProjectionForward");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projection")
	FName Param_ProjectionUp=TEXT("ProjectionUp");

	

private:
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* MID;

	void InitMID();
	void UpdateProjectionParams();
};