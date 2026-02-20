// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "ObstacleOcclusionComp.generated.h"


/*
 *  This will be used for detecting the obstacle hiding the character on the camera view
 *
 *  this is made in a separate component,
 *  -->because there are cases when there are multiple actors need to be shown in one camera other than player character
 *
 *  also, this will not use capsule but array of sphere collisions because of the curve made by curved world bender shader.
 */

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTER_API UObstacleOcclusionComp : public USceneComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UObstacleOcclusionComp();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
};
