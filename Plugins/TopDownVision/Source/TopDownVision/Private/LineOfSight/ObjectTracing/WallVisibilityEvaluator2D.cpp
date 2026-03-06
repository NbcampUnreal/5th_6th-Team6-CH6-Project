// Fill out your copyright notice in the Description page of Project Settings.


#include "LineOfSight/ObjectTracing/WallVisibilityEvaluator2D.h"
#include "LineOfSight/ObjectTracing/TopDown2DShapeComp.h"

DEFINE_LOG_CATEGORY(WallObstacleEvaluator);

bool UWallVisibilityEvaluator2D::EvaluateVisibility(
    UWorld* World,
    const FVector& ObserverLocation,
    const FVector& TargetLocation,
    UTopDown2DShapeComp* ShapeComp,
    ECollisionChannel WallChannel)
{
    if (!World || !ShapeComp)
    {
        UE_LOG(WallObstacleEvaluator, Warning,
            TEXT("EvaluateVisibility >> Invalid input: World=%s ShapeComp=%s"),
            World     ? TEXT("OK") : TEXT("NULL"),
            ShapeComp ? TEXT("OK") : TEXT("NULL"));
        return true; // fail open
    }

    const float TraceRadius = ShapeComp->GetTraceRadius();
    const float Spacing     = ShapeComp->GetPointSpacingDistance();

    if (TraceRadius <= KINDA_SMALL_NUMBER)
    {
        UE_LOG(WallObstacleEvaluator, Warning,
            TEXT("EvaluateVisibility >> TraceRadius is zero"));
        return true;
    }

    const int32 NumTraces = FMath::Max(1, FMath::RoundToInt(TraceRadius / Spacing));

    UE_LOG(WallObstacleEvaluator, Verbose,
        TEXT("EvaluateVisibility >> TraceRadius: %.1f | Spacing: %.1f | NumTraces: %d"),
        TraceRadius, Spacing, NumTraces);

    const TArray<FVector> EndPoints = BuildFanEndPoints(
        ObserverLocation, TargetLocation, TraceRadius, NumTraces);

    FCollisionQueryParams QueryParams;
    QueryParams.bTraceComplex = false;

    for (const FVector& EndPoint : EndPoints)
    {
        FHitResult Hit;
        const bool bBlocked = World->LineTraceSingleByChannel(
            Hit, ObserverLocation, EndPoint, WallChannel, QueryParams);

        UE_LOG(WallObstacleEvaluator, VeryVerbose,
            TEXT("EvaluateVisibility >> Trace to (%.1f, %.1f, %.1f) -> Blocked: %s"),
            EndPoint.X, EndPoint.Y, EndPoint.Z,
            bBlocked ? TEXT("YES") : TEXT("NO"));

        if (!bBlocked)
        {
            UE_LOG(WallObstacleEvaluator, Verbose,
                TEXT("EvaluateVisibility >> Clear trace found -> VISIBLE"));
            return true; // any single clear trace = visible
        }
    }

    UE_LOG(WallObstacleEvaluator, Verbose,
        TEXT("EvaluateVisibility >> All %d traces blocked -> HIDDEN"), NumTraces);

    return false;
}

// -------------------------------------------------------------------------- //

TArray<FVector> UWallVisibilityEvaluator2D::BuildFanEndPoints(
    const FVector& ObserverLocation,
    const FVector& TargetLocation,
    float TraceRadius,
    int32 NumTraces)
{
    TArray<FVector> EndPoints;
    EndPoints.Reserve(NumTraces);

    const FVector ToTarget   = TargetLocation - ObserverLocation;
    const float   Distance   = ToTarget.Size2D();
    const FVector DirToTarget = ToTarget.GetSafeNormal2D();

    // Half angle of the fan — wider for larger shapes or closer targets
    const float HalfAngleRad = FMath::Atan2(TraceRadius, FMath::Max(Distance, 1.f));

    if (NumTraces == 1)
    {
        // Single trace goes straight to target
        EndPoints.Add(TargetLocation);
        return EndPoints;
    }

    // Spread NumTraces evenly across the fan
    for (int32 i = 0; i < NumTraces; ++i)
    {
        const float Alpha = (float)i / (float)(NumTraces - 1); // 0..1
        const float Angle = FMath::Lerp(-HalfAngleRad, HalfAngleRad, Alpha);

        // Rotate direction by angle around Z axis
        const float Cos = FMath::Cos(Angle);
        const float Sin = FMath::Sin(Angle);
        const FVector TraceDir(
            DirToTarget.X * Cos - DirToTarget.Y * Sin,
            DirToTarget.X * Sin + DirToTarget.Y * Cos,
            0.f);

        // End point at target distance + radius so traces fully cross the target face
        EndPoints.Add(ObserverLocation + TraceDir * (Distance + TraceRadius));
    }

    UE_LOG(WallObstacleEvaluator, VeryVerbose,
        TEXT("BuildFanEndPoints >> Distance: %.1f | HalfAngle: %.2f deg | Traces: %d"),
        Distance,
        FMath::RadiansToDegrees(HalfAngleRad),
        NumTraces);

    return EndPoints;
}