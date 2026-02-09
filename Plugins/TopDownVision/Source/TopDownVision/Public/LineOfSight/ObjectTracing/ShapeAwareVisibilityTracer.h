// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ShapeAwareVisibilityTracer.generated.h"




/**
 *   After the Sphere Collision Comp overlaps with the AActor*
 *   -> use this to check if the target is visible or not to the player character
 *
 *   only for returning bool value for visibility by linetracing
 */

//ForwardDeclares
class UPrimitiveComponent;
class USphereComponent;
class UCapsuleComponent;
class UBoxComponent;

//Log

TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(VisibilityTrace, Log, All);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UShapeAwareVisibilityTracer : public UObject
{
	GENERATED_BODY()
	
public:
	bool IsTargetVisible(
		UWorld* ContextWorld,
		const FVector& ObserverLocation,
		UPrimitiveComponent* TargetShape,
		float MaxDistance,
		ECollisionChannel ObstacleChannel,
		const TArray<AActor*>& IgnoredActors,
		bool bDrawDebugLine=false,
		float RayGapDegrees = 5.f);//default 5 degrees

private:
	
	bool TraceVisibilityRay(
		UWorld* ContextWorld,
		const FVector& Start,
		const FVector& End,
		const TArray<AActor*>& IgnoredActors,
		ECollisionChannel ObstacleChannel,
		bool bDrawDebugLine=false) const;

	
	bool ComputeAngularSpan(
		const FVector& ObserverLocation,
		UPrimitiveComponent* ShapeComp,
		//out
		float& OutLeftRad,
		float& OutRightRad) const;
	
	//==== Shape aware internal functions ====//
	
	// sphere
	bool ComputeSphereSpan(
		const FVector& ObserverLocation,
		const USphereComponent* Sphere,
		//out
		float& OutLeftRad,
		float& OutRightRad) const;

	//capsule
	bool ComputeCapsuleSpan(
		const FVector& ObserverLocation,
		const UCapsuleComponent* Capsule,
		//out
		float& OutLeftRad,
		float& OutRightRad) const;
	
	//box
	bool ComputeBoxSpan(
		const FVector& ObserverLocation,
		const UBoxComponent* Box,
		//out
		float& OutLeftRad,
		float& OutRightRad) const;
	
	//bound shape
	bool ComputeBoundsSpan(
		const FVector& ObserverLocation,
		const UPrimitiveComponent* ShapeComp,
		//out
		float& OutLeftRad,
		float& OutRightRad) const;


};
