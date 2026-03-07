// Fill out your copyright notice in the Description page of Project Settings.

#include "LineOfSight/ObjectTracing/VolumeVisibilityEvaluator2D.h"

#include "Engine/TextureRenderTarget2D.h"
#include "LineOfSight/ObjectTracing/TopDown2DShapeComp.h"

DEFINE_LOG_CATEGORY(VolumeObstacleEvaluator);

bool UVolumeVisibilityEvaluator2D::EvaluateVisibility(
    UTextureRenderTarget2D* ObstacleRT,
    float MaxRange,
    const FVector& ObserverLocation,
    const FVector& TargetLocation,
    UTopDown2DShapeComp* ShapeComp,
    float OcclusionThreshold)
{
    if (!ObstacleRT || !ShapeComp)
    {
        UE_LOG(VolumeObstacleEvaluator, Warning,
            TEXT("EvaluateVisibility >> Invalid input: ObstacleRT=%s ShapeComp=%s"),
            ObstacleRT ? TEXT("OK") : TEXT("NULL"),
            ShapeComp  ? TEXT("OK") : TEXT("NULL"));
        return true;
    }

    if (MaxRange <= KINDA_SMALL_NUMBER)
    {
        UE_LOG(VolumeObstacleEvaluator, Warning,
            TEXT("EvaluateVisibility >> MaxRange is zero or invalid"));
        return true;
    }

    const TArray<FVector2D>& InnerPoints = ShapeComp->GetInnerSamplePoints();
    if (InnerPoints.IsEmpty())
    {
        UE_LOG(VolumeObstacleEvaluator, Warning,
            TEXT("EvaluateVisibility >> No inner sample points on ShapeComp"));
        return true;
    }

    FRenderTarget* RTResource = ObstacleRT->GameThread_GetRenderTargetResource();
    if (!RTResource)
    {
        UE_LOG(VolumeObstacleEvaluator, Warning,
            TEXT("EvaluateVisibility >> Failed to get RT resource"));
        return true;
    }

    TArray<FColor> Pixels;
    if (!RTResource->ReadPixels(Pixels))
    {
        UE_LOG(VolumeObstacleEvaluator, Warning,
            TEXT("EvaluateVisibility >> ReadPixels failed"));
        return true;
    }

    const int32 RTWidth  = ObstacleRT->SizeX;
    const int32 RTHeight = ObstacleRT->SizeY;

    UE_LOG(VolumeObstacleEvaluator, Verbose,
        TEXT("EvaluateVisibility >> RT: %dx%d | Points: %d | MaxRange: %.1f | Threshold: %.2f"),
        RTWidth, RTHeight, InnerPoints.Num(), MaxRange, OcclusionThreshold);

    int32 OccludedCount = 0;

    for (const FVector2D& LocalPoint : InnerPoints)
    {
        const FVector WorldPoint = TargetLocation + FVector(LocalPoint.X, LocalPoint.Y, 0.f);
        const FVector2D UV = WorldToObstacleUV(WorldPoint, ObserverLocation, MaxRange);

        const bool bOccluded = SampleGChannel(Pixels, RTWidth, RTHeight, UV.X, UV.Y);

        UE_LOG(VolumeObstacleEvaluator, VeryVerbose,
            TEXT("EvaluateVisibility >> Point(%.1f, %.1f) -> UV(%.3f, %.3f) -> Occluded: %s"),
            LocalPoint.X, LocalPoint.Y,
            UV.X, UV.Y,
            bOccluded ? TEXT("YES") : TEXT("NO"));

        if (bOccluded)
            OccludedCount++;
    }

    const float OccludedRatio = (float)OccludedCount / (float)InnerPoints.Num();
    const bool bVisible = OccludedRatio < OcclusionThreshold;

    UE_LOG(VolumeObstacleEvaluator, Verbose,
        TEXT("EvaluateVisibility >> Occluded: %d/%d (%.1f%%) | Threshold: %.1f%% | Result: %s"),
        OccludedCount, InnerPoints.Num(),
        OccludedRatio * 100.f,
        OcclusionThreshold * 100.f,
        bVisible ? TEXT("VISIBLE") : TEXT("HIDDEN"));

    return bVisible;
}

// -------------------------------------------------------------------------- //

FVector2D UVolumeVisibilityEvaluator2D::WorldToObstacleUV(
    const FVector& WorldPoint,
    const FVector& ObserverLocation,
    float MaxRange)
{
    const FVector Delta = WorldPoint - ObserverLocation;

    const float U = FMath::Clamp((Delta.X / MaxRange + 1.f) * 0.5f, 0.f, 1.f);
    const float V = FMath::Clamp((Delta.Y / MaxRange + 1.f) * 0.5f, 0.f, 1.f);

    UE_LOG(VolumeObstacleEvaluator, VeryVerbose,
        TEXT("WorldToObstacleUV >> Delta(%.1f, %.1f) -> UV(%.3f, %.3f)"),
        Delta.X, Delta.Y, U, V);

    return FVector2D(U, V);
}

bool UVolumeVisibilityEvaluator2D::SampleGChannel(
    const TArray<FColor>& Pixels,
    int32 RTWidth,
    int32 RTHeight,
    float U,
    float V)
{
    const int32 PixelX = FMath::Clamp(FMath::FloorToInt(U * RTWidth),  0, RTWidth  - 1);
    const int32 PixelY = FMath::Clamp(FMath::FloorToInt(V * RTHeight), 0, RTHeight - 1);
    const int32 Index  = PixelY * RTWidth + PixelX;

    if (!Pixels.IsValidIndex(Index))
    {
        UE_LOG(VolumeObstacleEvaluator, Warning,
            TEXT("SampleGChannel >> Pixel index %d out of range (total: %d) at UV(%.3f, %.3f)"),
            Index, Pixels.Num(), U, V);
        return false;
    }

    const bool bOccluded = Pixels[Index].G > 0;

    UE_LOG(VolumeObstacleEvaluator, VeryVerbose,
        TEXT("SampleGChannel >> Pixel[%d,%d] G=%d -> Occluded: %s"),
        PixelX, PixelY, Pixels[Index].G,
        bOccluded ? TEXT("YES") : TEXT("NO"));

    return bOccluded;
}