// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CurvedWorldSubsystem.h"

/**
 *  This is for applying same math to the cpp as the material curve used for WPO
 */
class WORLDBENDER_API FCurvedWorldUtil
{
public:

	//Base
	static void CalculateCurvedWorldTransform(
	  const FVector& WorldPos,
	  const FVector& Origin,
	  float CurveX,
	  float CurveY,
	  float BendWeight,
	  const FVector& RightVector,
	  const FVector& ForwardVector,
	  //out
	  FVector& OutOffset,
	  FRotator& OutRotation);
	
	static FVector CalculateCurvedWorldOffset(
	   const FVector& WorldPos,
	   const FVector& Origin,
	   float CurveX,
	   float CurveY,
	   float BendWeight,
	   const FVector& RightVector,
	   const FVector& ForwardVector);



//========== Projection Helper ==========//
	
	static FVector VisualCurvedToWorld( // this is for inverting the math and get world correct location
		const FVector& VisualPos,
		const UCurvedWorldSubsystem* CurvedWorld);

	static FVector WorldToVisualCurved(// this do the opposite
		const FVector& WorldPos,
		const UCurvedWorldSubsystem* CurvedWorld);

	//CurvedWorld Tracer
	static bool GetHitResultUnderCursorCorrected(
			APlayerController* PlayerController,
			const UCurvedWorldSubsystem* CurvedWorld,
			FHitResult& OutHitResult,
			ECollisionChannel TraceChannel);
    
	//Generate points from start and end of the curve path
	static TArray<FVector> GenerateCurvedPathPoints(
		const FVector& StartPos,
		const FVector& EndPos,
		int32 NumPoints,
		const UCurvedWorldSubsystem* CurvedWorld);
};
