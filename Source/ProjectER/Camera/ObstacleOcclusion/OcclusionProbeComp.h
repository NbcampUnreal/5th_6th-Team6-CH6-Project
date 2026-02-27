// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "CurvedWorldSubsystem.h"
#include "OcclusionProbeComp.generated.h"


/*
 *  This will be used for detecting the obstacle hiding the character on the camera view
 *
 *  this is made in a separate component,
 *  -->because there are cases when there are multiple actors need to be shown in one camera other than player character
 *
 *  also, this will not use capsule but array of sphere collisions because of the curve made by curved world bender shader.
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
	virtual void TickComponent(float DeltaTime,
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
	APlayerCameraManager* CameraManager;

	UPROPERTY()
	UCurvedWorldSubsystem* CurvedWorldSubsystem;
};
