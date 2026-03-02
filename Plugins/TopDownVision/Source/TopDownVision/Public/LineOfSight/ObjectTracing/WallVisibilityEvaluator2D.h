// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WallVisibilityEvaluator2D.generated.h"


/**
 * Evaluates whether a target is hidden by a wall obstacle using fan-shaped line traces.
 * Traces fan from observer toward target, spread derived from target's shape radius.
 * Any single unblocked trace = visible.
 */



// FD
class UTopDown2DShapeComp;

TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(WallObstacleEvaluator, Log, All);

UCLASS()
class TOPDOWNVISION_API UWallVisibilityEvaluator2D : public UObject
{
	GENERATED_BODY()

public:

	
	UFUNCTION(BlueprintCallable, Category="WallVisibility")
	static bool EvaluateVisibility(
		UWorld* World,
		const FVector& ObserverLocation,
		const FVector& TargetLocation,
		UTopDown2DShapeComp* ShapeComp,
		ECollisionChannel WallChannel = ECC_Visibility);

private:
	/** Build fan trace end points spread across the target face. */
	static TArray<FVector> BuildFanEndPoints(
		const FVector& ObserverLocation,
		const FVector& TargetLocation,
		float TraceRadius,
		int32 NumTraces);
};