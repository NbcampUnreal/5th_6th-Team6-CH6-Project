// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 *  This is for applying same math to the cpp as the material curve used for WPO
 */
class WORLDBENDER_API FCurvedWorldUtil
{
public:
	
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
};
