#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "VolumeVisibilityEvaluator2D.generated.h"



/**
 *  This is for topdown 2d use case method. it will use the obstacle RT and sample the pixel G value to check obstacle occlusion
 *
 *
 *  Fuck yeah
 */

//Log
TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(VolumeObstacleEvaluator, Log, All);


// ==== FD ==== //
class UTextureRenderTarget2D;
class UTopDown2DShapeComp;

UCLASS()
class TOPDOWNVISION_API UVolumeVisibilityEvaluator2D : public UObject
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable, Category="VolumeVisibility")
    static bool EvaluateVisibility(
        UTextureRenderTarget2D* ObstacleRT,
        float MaxRange,
        const FVector& ObserverLocation,
        const FVector& TargetLocation,
        UTopDown2DShapeComp* ShapeComp,
        float OcclusionThreshold = 0.8f);

private:
    /** Sample G channel at a given UV in the pixel buffer. Returns true if occluded. */
    static bool SampleGChannel(
        const TArray<FColor>& Pixels,
        int32 RTWidth,
        int32 RTHeight,
        float U,
        float V);

    /** Project a world point into the observer's obstacle RT UV space. */
    static FVector2D WorldToObstacleUV(
        const FVector& WorldPoint,
        const FVector& ObserverLocation,
        float MaxRange);
};