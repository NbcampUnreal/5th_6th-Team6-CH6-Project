// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShapeAwareVisibilityComp.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UShapeAwareVisibilityComp : public UActorComponent
{
	GENERATED_BODY()

public:
	/*void UpdateVisibility(
	  const FVector& ObserverLocation,
	  float VisionRange,
	  TArray<AActor*>& OutVisibleActors
  );


private:
	void SampleTarget(
		UPrimitiveComponent* ShapeComp,
		const FVector& ObserverLocation,
		//out
		TArray<FHitResult>& OutHits);*/

	//shapes
/*
 *  make a function to return the edge collision location, so that it can be used for first, and last linetrace.
 *  and between those lines will be filled with other lines
 */

protected:

	float RayGapDegree=10.f;// make lines for every 10 degrees-> will be modified to match actual range
	
	UPROPERTY()
	TArray<AActor*> DetectedTargets;// this is where the detected actors are stored.
	

};
