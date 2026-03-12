#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "CurvedWorldSubsystem.h"
#include "OcclusionProbeComp.generated.h"

/*
 * Used for detecting obstacles hiding the character on the camera view.
 * Made as a separate component — multiple actors may need to be shown in one camera.
 * Uses an array of sphere collisions instead of a capsule
 * to account for the curve made by the curved world bender shader.
 */

class USphereComponent;
class APlayerCameraManager;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTER_API UOcclusionProbeComp : public USceneComponent
{
	GENERATED_BODY()

public:

	UOcclusionProbeComp();

protected:

	virtual void BeginPlay() override;

public:

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:

	void InitializeProbes();
	void UpdateProbes();
	float CalculateScreenProjectedRadius() const;

private:

	UPROPERTY(EditAnywhere, Category="Occlusion")
	int32 NumProbes = 12;

	UPROPERTY(EditAnywhere, Category="Occlusion")
	float MinSphereRadius = 25.f;

	UPROPERTY(EditAnywhere, Category="Occlusion")
	float MaxSphereRadius = 200.f;

	UPROPERTY(EditAnywhere, Category="Occlusion")
	TEnumAsByte<ECollisionChannel> OcclusionProbeChannel = ECC_GameTraceChannel3;

	UPROPERTY()
	TArray<USphereComponent*> ProbeSpheres;

	UPROPERTY()
	APlayerCameraManager* CameraManager = nullptr;

	UPROPERTY()
	UCurvedWorldSubsystem* CurvedWorldSubsystem = nullptr;
};