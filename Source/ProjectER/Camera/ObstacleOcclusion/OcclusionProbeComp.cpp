// Fill out your copyright notice in the Description page of Project Settings.


#include "OcclusionProbeComp.h"


#include "FCurvedWorldUtil.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "DrawDebugHelpers.h"


UOcclusionProbeComp::UOcclusionProbeComp()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UOcclusionProbeComp::BeginPlay()
{
    Super::BeginPlay();

    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (PC)
    {
        CameraManager = PC->PlayerCameraManager;
    }

    CurvedWorldSubsystem = GetWorld()->GetSubsystem<UCurvedWorldSubsystem>();
}

void UOcclusionProbeComp::TickComponent(float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    RunSweeps();
    ProcessHitDiff();
}


void UOcclusionProbeComp::RunSweeps()
{
    if (!CameraManager || !CurvedWorldSubsystem)
        return;

    FVector CameraLoc  = CameraManager->GetCameraLocation();
    FVector TargetLoc  = GetComponentLocation();
    float   SweepRadius = CalculateSweepRadius();

    // Generate curved path points — one extra point so we have N segments between N+1 points
    TArray<FVector> PathPoints = FCurvedWorldUtil::GenerateCurvedPathPoints(
        CameraLoc,
        TargetLoc,
        NumSegments + 1,
        CurvedWorldSubsystem,
        ECurveMathType::ZHeightOnly);

    if (PathPoints.Num() < 2)
        return;

    // Params shared across all sweeps
    FCollisionShape SphereShape = FCollisionShape::MakeSphere(SweepRadius);

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(GetOwner());
    QueryParams.bTraceComplex = false;

    // Carry over previous frame's hits, then rebuild CurrentHits fresh
    PreviousHits = MoveTemp(CurrentHits);
    CurrentHits.Reset();

    for (int32 i = 0; i < PathPoints.Num() - 1; ++i)
    {
        const FVector& Start = PathPoints[i];
        const FVector& End   = PathPoints[i + 1];

        TArray<FHitResult> HitResults;

        GetWorld()->SweepMultiByChannel(
            HitResults,
            Start,
            End,
            FQuat::Identity,
            OcclusionProbeChannel,
            SphereShape,
            QueryParams);

        for (const FHitResult& Hit : HitResults)
        {
            if (UPrimitiveComponent* HitComp = Hit.GetComponent())
            {
                CurrentHits.Add(HitComp);
            }
        }

#if ENABLE_DRAW_DEBUG
        if (bDrawDebug)
        {
            DrawDebugSphere(GetWorld(), Start, SweepRadius, 8, FColor::Yellow, false, 0.f);
            DrawDebugLine(GetWorld(), Start, End, FColor::Cyan, false, 0.f);
        }
#endif
    }

#if ENABLE_DRAW_DEBUG
    if (bDrawDebug && PathPoints.Num() > 0)
    {
        DrawDebugSphere(GetWorld(), PathPoints.Last(), SweepRadius, 8, FColor::Yellow, false, 0.f);
    }
#endif
}


void UOcclusionProbeComp::ProcessHitDiff()
{
    // --- Occlusion BEGIN: in current but not in previous ---
    for (const TWeakObjectPtr<UPrimitiveComponent>& WeakComp : CurrentHits)
    {
        if (!PreviousHits.Contains(WeakComp))
        {
            if (UPrimitiveComponent* Comp = WeakComp.Get())
            {
                // TODO: call IOcclusionTarget::Execute_OnOcclusionBegin on Comp->GetOwner()
                // Left as a stub until the interface is added in the next step
                (void)Comp;
            }
        }
    }

    // --- Occlusion END: in previous but not in current ---
    for (const TWeakObjectPtr<UPrimitiveComponent>& WeakComp : PreviousHits)
    {
        if (!CurrentHits.Contains(WeakComp))
        {
            if (UPrimitiveComponent* Comp = WeakComp.Get())
            {
                // TODO: call IOcclusionTarget::Execute_OnOcclusionEnd on Comp->GetOwner()
                (void)Comp;
            }
        }
    }
}


float UOcclusionProbeComp::CalculateSweepRadius() const
{
    if (!CameraManager)
        return MinSweepRadius;

    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC)
        return MinSweepRadius;

    int32 ViewportX, ViewportY;
    PC->GetViewportSize(ViewportX, ViewportY);
    if (ViewportX == 0 || ViewportY == 0)
        return MinSweepRadius;

    FVector ActorLocation = GetOwner()->GetActorLocation();

    FBox TargetBounds = GetOwner()->GetComponentsBoundingBox();
    float ApproxRadius = TargetBounds.GetExtent().Size();

    FVector WorldEdge = ActorLocation + FVector(ApproxRadius, 0.f, 0.f);

    FVector2D ScreenCenter, ScreenEdge;
    if (!PC->ProjectWorldLocationToScreen(ActorLocation, ScreenCenter) ||
        !PC->ProjectWorldLocationToScreen(WorldEdge, ScreenEdge))
        return MinSweepRadius;

    float PixelRadius = FVector2D::Distance(ScreenCenter, ScreenEdge);
    float NDCRadius   = PixelRadius / static_cast<float>(ViewportY);

    float DistanceToCamera = FVector::Distance(
        ActorLocation,
        CameraManager->GetCameraLocation());

    float HalfFOVRad    = FMath::DegreesToRadians(CameraManager->GetFOVAngle() * 0.5f);
    float FrustumHeight = 2.f * DistanceToCamera * FMath::Tan(HalfFOVRad);

    float WorldRadius = NDCRadius * FrustumHeight;

    return FMath::Clamp(WorldRadius, MinSweepRadius, MaxSweepRadius);
}
