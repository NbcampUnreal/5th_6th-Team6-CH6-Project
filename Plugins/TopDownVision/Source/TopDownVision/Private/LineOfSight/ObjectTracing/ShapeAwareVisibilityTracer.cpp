// Fill out your copyright notice in the Description page of Project Settings.


#include "LineOfSight/ObjectTracing/ShapeAwareVisibilityTracer.h"

//Shapes
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"

// Debug
#include "DrawDebugHelpers.h"
#include "TopDownVisionDebug.h"

//Log
DEFINE_LOG_CATEGORY(VisibilityTrace);

bool UShapeAwareVisibilityTracer::IsTargetVisible(
    UWorld* ContextWorld,
    const FVector& ObserverLocation,
    UPrimitiveComponent* TargetShape,
    float MaxDistance,
    ECollisionChannel ObstacleChannel,
    const TArray<AActor*>& IgnoredActors,
    bool bDrawDebugLine,
    float RayGapDegrees)
{
    const FString Ctx = TopDownVisionDebug::GetClientDebugName(this);

    if (!ContextWorld || !TargetShape)
    {
        UE_LOG(VisibilityTrace, Warning,
            TEXT("%s UShapeAwareVisibilityTracer::IsTargetVisible >> null World or TargetShape"),
            *Ctx);
        return false;
    }

    AActor* TargetActor = TargetShape->GetOwner();
    if (!TargetActor)
    {
        UE_LOG(VisibilityTrace, Warning,
            TEXT("%s UShapeAwareVisibilityTracer::IsTargetVisible >> TargetShape has no owner"),
            *Ctx);
        return false;
    }

    // Quick distance check before doing any trace work
    const FVector TargetCenter = TargetShape->GetComponentLocation();
    const float DistSq = FVector::DistSquared(ObserverLocation, TargetCenter);
    if (DistSq > FMath::Square(MaxDistance))
    {
        UE_LOG(VisibilityTrace, Verbose,
            TEXT("%s UShapeAwareVisibilityTracer::IsTargetVisible >> [%s] out of range"),
            *Ctx, *TargetActor->GetName());
        return false;
    }

    // Compute bounding span
    float CenterRad = 0.f;
    float HalfAngleRad = 0.f;

    if (!ComputeBoundingSpanRadians(ObserverLocation, TargetShape, CenterRad, HalfAngleRad))
    {
        UE_LOG(VisibilityTrace, Warning,
            TEXT("%s UShapeAwareVisibilityTracer::IsTargetVisible >> [%s] failed to compute bounding span — observer may be inside target"),
            *Ctx, *TargetActor->GetName());
        return false;
    }

    const float RayGapRad = FMath::DegreesToRadians(RayGapDegrees);
    const float LeftRad   = CenterRad - HalfAngleRad;
    const float RightRad  = CenterRad + HalfAngleRad;

    // Always fire at least one ray at the center even if the span is very narrow
    const int32 NumRays = FMath::Max(1, FMath::RoundToInt((RightRad - LeftRad) / RayGapRad));

    UE_LOG(VisibilityTrace, Verbose,
        TEXT("%s UShapeAwareVisibilityTracer::IsTargetVisible >> [%s] span=%.2f deg | rays=%d"),
        *Ctx,
        *TargetActor->GetName(),
        FMath::RadiansToDegrees(HalfAngleRad * 2.f),
        NumRays);

    for (int32 i = 0; i <= NumRays; ++i)
    {
        // Lerp across the span, always including both edges
        const float T = (NumRays == 0) ? 0.5f : (float)i / NumRays;
        const float Angle = FMath::Lerp(LeftRad, RightRad, T);

        const FVector RayDir = FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
        const FVector RayEnd = ObserverLocation + RayDir * MaxDistance;

        if (TraceVisibilityRay(
            ContextWorld,
            ObserverLocation,
            RayEnd,
            TargetActor,
            ObstacleChannel,
            IgnoredActors,
            bDrawDebugLine))
        {
            UE_LOG(VisibilityTrace, Verbose,
                TEXT("%s UShapeAwareVisibilityTracer::IsTargetVisible >> [%s] visible via ray %d / %d"),
                *Ctx, *TargetActor->GetName(), i, NumRays);
            return true;
        }
    }

    UE_LOG(VisibilityTrace, Verbose,
        TEXT("%s UShapeAwareVisibilityTracer::IsTargetVisible >> [%s] all %d rays blocked"),
        *Ctx, *TargetActor->GetName(), NumRays + 1);

    return false;
}


//  Private — Bounding Span

bool UShapeAwareVisibilityTracer::ComputeBoundingSpanRadians(
    const FVector& ObserverLocation,
    UPrimitiveComponent* TargetShape,
    float& OutCenterRad,
    float& OutHalfAngleRad) const
{
    const FString Ctx = TopDownVisionDebug::GetClientDebugName(this);

    // Get bounding sphere in world space
    FBoxSphereBounds Bounds = TargetShape->CalcBounds(TargetShape->GetComponentTransform());
    const FVector  TargetCenter  = Bounds.Origin;
    const float    BoundRadius   = Bounds.SphereRadius;

    // Flatten to XY — Z is irrelevant for top-down
    const FVector2D ObserverXY(ObserverLocation.X, ObserverLocation.Y);
    const FVector2D TargetXY(TargetCenter.X, TargetCenter.Y);

    const FVector2D ToTarget = TargetXY - ObserverXY;
    const float Distance = ToTarget.Size();

    // Observer is inside or on the bounding sphere — can't compute a meaningful span
    if (Distance <= BoundRadius)
    {
        UE_LOG(VisibilityTrace, Warning,
            TEXT("%s UShapeAwareVisibilityTracer::ComputeBoundingSpanRadians >> observer inside bounding sphere of [%s]"),
            *Ctx, *TargetShape->GetOwner()->GetName());
        return false;
    }

    OutCenterRad   = FMath::Atan2(ToTarget.Y, ToTarget.X);
    OutHalfAngleRad = FMath::Asin(FMath::Clamp(BoundRadius / Distance, -1.f, 1.f));

    UE_LOG(VisibilityTrace, Verbose,
        TEXT("%s UShapeAwareVisibilityTracer::ComputeBoundingSpanRadians >> [%s] dist=%.1f | radius=%.1f | halfAngle=%.2f deg"),
        *Ctx,
        *TargetShape->GetOwner()->GetName(),
        Distance,
        BoundRadius,
        FMath::RadiansToDegrees(OutHalfAngleRad));

    return true;
}

//  Single Ray Trace

bool UShapeAwareVisibilityTracer::TraceVisibilityRay(
    UWorld* ContextWorld,
    const FVector& Start,
    const FVector& End,
    AActor* TargetActor,
    ECollisionChannel ObstacleChannel,
    const TArray<AActor*>& IgnoredActors,
    bool bDrawDebugLine) const
{
    FCollisionQueryParams Params;
    Params.bTraceComplex = false;
    Params.AddIgnoredActors(IgnoredActors);

    FHitResult HitResult;
    const bool bHit = ContextWorld->LineTraceSingleByChannel(
        HitResult,
        Start,
        End,
        ObstacleChannel,
        Params);

    // Visible if the first hit is the target actor
    const bool bTargetHit = bHit && HitResult.GetActor() == TargetActor;
    
    if (bDrawDebugLine)
    {
        const FColor LineColor = bTargetHit ? FColor::Green : FColor::Red;
        DrawDebugLine(
            ContextWorld,
            Start,
            bHit ? HitResult.ImpactPoint : End, LineColor,
            false,
            0.1f,
            0,
            0.5f);

        if (bHit)
        {
            DrawDebugSphere(ContextWorld, HitResult.ImpactPoint, 4.f, 4,
                bTargetHit ? FColor::Green : FColor::Red, false, 0.1f);
        }
    }


    return bTargetHit;
}