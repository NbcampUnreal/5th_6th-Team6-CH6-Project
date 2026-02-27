// Fill out your copyright notice in the Description page of Project Settings.

#include "OcclusionProbeComp.h"

#include "FCurvedWorldUtil.h"
#include "TopDownVision/Public/ObstacleOcclusion/PhysicallOcclusion/OcclusionInterface.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "DrawDebugHelpers.h"

DEFINE_LOG_CATEGORY(OcclusionProbeComp);


UOcclusionProbeComp::UOcclusionProbeComp()
{
    // Tick disabled — UpdateOcclusionProbe() is called manually
    PrimaryComponentTick.bCanEverTick = false;
}

void UOcclusionProbeComp::BeginPlay()
{
    Super::BeginPlay();

    //SetCameraManager();//set camera -> temp for bp testing
    
    CurvedWorldSubsystem = GetWorld()->GetSubsystem<UCurvedWorldSubsystem>();

    if (!CurvedWorldSubsystem)
    {
        UE_LOG(OcclusionProbeComp, Error,
            TEXT("UOcclusionProbeComp::BeginPlay >> CurvedWorldSubsystem is null"));
        return;
    }

    UE_LOG(OcclusionProbeComp, Log,
        TEXT("UOcclusionProbeComp::BeginPlay >> Initialized on %s"),
        *GetOwner()->GetName());
}


// ------------------------------------------------------------
// Public — called manually from custom tick
// ------------------------------------------------------------

void UOcclusionProbeComp::UpdateOcclusionProbe()
{
    RunSweeps();
    ProcessHitDiff();
}

void UOcclusionProbeComp::SetCameraManager()
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC)
    {
        UE_LOG(OcclusionProbeComp, Error,
            TEXT("UOcclusionProbeComp::BeginPlay >> Failed to get PlayerController"));
        return;
    }

    CameraManager = PC->PlayerCameraManager;

    if (!CameraManager)
    {
        UE_LOG(OcclusionProbeComp, Error,
            TEXT("UOcclusionProbeComp::BeginPlay >> PlayerCameraManager is null"));
        return;
    }

}


// ------------------------------------------------------------
// Private
// ------------------------------------------------------------

void UOcclusionProbeComp::RunSweeps()
{
    if (!CameraManager || !CurvedWorldSubsystem)
    {
        UE_LOG(OcclusionProbeComp, Error,
            TEXT("UOcclusionProbeComp::RunSweeps >> Invalid CameraManager or CurvedWorldSubsystem"));
        return;
    }

    FVector CameraLoc = CameraManager->GetCameraLocation();
    FVector TargetLoc = GetComponentLocation();
    float SweepRadius = CalculateSweepRadius();

    // Offset target toward camera by half radius so the last sphere sits
    // just in front of the target rather than centered on it.
    // This prevents false occlusion hits directly behind the target.
    const FVector CameraToTarget = (TargetLoc - CameraLoc).GetSafeNormal();
    TargetLoc -= CameraToTarget * SweepRadius;

    // Derive segment count from path length so coverage stays consistent at any distance
    const float  PathLength  = FVector::Distance(CameraLoc, TargetLoc);
    const int32  NumSegments = FMath::Clamp(
        FMath::CeilToInt(PathLength / SegmentLength),
        MinSegments,
        MaxSegments);

    UE_LOG(OcclusionProbeComp, Verbose,
        TEXT("UOcclusionProbeComp::RunSweeps >> PathLength: %.1f | Segments: %d | SweepRadius: %.1f"),
        PathLength, NumSegments, SweepRadius);

    // N+1 points gives N segments
    TArray<FVector> PathPoints = FCurvedWorldUtil::GenerateCurvedPathPoints(
        CameraLoc,
        TargetLoc,
        NumSegments + 1,
        CurvedWorldSubsystem,
        ECurveMathType::ZHeightOnly);

    if (PathPoints.Num() < 2)
    {
        UE_LOG(OcclusionProbeComp, Warning,
            TEXT("UOcclusionProbeComp::RunSweeps >> Not enough path points generated (%d)"),
            PathPoints.Num());
        return;
    }

    FCollisionShape       SphereShape = FCollisionShape::MakeSphere(SweepRadius);
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(GetOwner());
    QueryParams.bTraceComplex = false;

    // Swap hit sets — previous frame becomes PreviousHits, CurrentHits is rebuilt fresh
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

                UE_LOG(OcclusionProbeComp, Verbose,
                    TEXT("UOcclusionProbeComp::RunSweeps >> Hit: %s on %s"),
                    *HitComp->GetName(),
                    *HitComp->GetOwner()->GetName());
            }
        }

        if (bDrawDebug)
        {
            DrawDebugSphere(GetWorld(), Start, SweepRadius, 8, FColor::Yellow, false, 0.f);
            DrawDebugLine(GetWorld(),   Start, End, FColor::Cyan, false, 0.f);
        }
    }

    if (bDrawDebug && PathPoints.Num() > 0)
    {
        DrawDebugSphere(GetWorld(), PathPoints.Last(), SweepRadius, 8, FColor::Yellow, false, 0.f);
    }
}


void UOcclusionProbeComp::ProcessHitDiff()
{
    // --- Occlusion ENTER: in current but not in previous ---
    for (const TWeakObjectPtr<UPrimitiveComponent>& WeakComp : CurrentHits)
    {
        if (!PreviousHits.Contains(WeakComp))
        {
            if (UPrimitiveComponent* Comp = WeakComp.Get())
            {
                AActor* Owner = Comp->GetOwner();
                if (Owner && Owner->Implements<UOcclusionInterface>())
                {
                    IOcclusionInterface::Execute_OnOcclusionEnter(Owner, Comp);

                    UE_LOG(OcclusionProbeComp, Log,
                        TEXT("UOcclusionProbeComp::ProcessHitDiff >> ENTER: %s"),
                        *Owner->GetName());
                }
            }
        }
    }

    // --- Occlusion EXIT: in previous but not in current ---
    for (const TWeakObjectPtr<UPrimitiveComponent>& WeakComp : PreviousHits)
    {
        if (!CurrentHits.Contains(WeakComp))
        {
            if (UPrimitiveComponent* Comp = WeakComp.Get())
            {
                AActor* Owner = Comp->GetOwner();
                if (Owner && Owner->Implements<UOcclusionInterface>())
                {
                    IOcclusionInterface::Execute_OnOcclusionExit(Owner, Comp);

                    UE_LOG(OcclusionProbeComp, Log,
                        TEXT("UOcclusionProbeComp::ProcessHitDiff >> EXIT: %s"),
                        *Owner->GetName());
                }
            }
        }
    }
}


float UOcclusionProbeComp::CalculateSweepRadius() const
{
    if (!CameraManager)
    {
        UE_LOG(OcclusionProbeComp, Warning,
            TEXT("UOcclusionProbeComp::CalculateSweepRadius >> No CameraManager, returning MinSweepRadius"));
        return MinSweepRadius;
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC)
    {
        UE_LOG(OcclusionProbeComp, Warning,
            TEXT("UOcclusionProbeComp::CalculateSweepRadius >> No PlayerController, returning MinSweepRadius"));
        return MinSweepRadius;
    }

    int32 ViewportX, ViewportY;
    PC->GetViewportSize(ViewportX, ViewportY);
    if (ViewportX == 0 || ViewportY == 0)
    {
        UE_LOG(OcclusionProbeComp, Warning,
            TEXT("UOcclusionProbeComp::CalculateSweepRadius >> Invalid viewport size, returning MinSweepRadius"));
        return MinSweepRadius;
    }

    FVector ActorLocation = GetOwner()->GetActorLocation();

    FBox    TargetBounds = GetOwner()->GetComponentsBoundingBox();
    float   ApproxRadius = TargetBounds.GetExtent().Size();
    FVector WorldEdge    = ActorLocation + FVector(ApproxRadius, 0.f, 0.f);

    FVector2D ScreenCenter, ScreenEdge;
    if (!PC->ProjectWorldLocationToScreen(ActorLocation, ScreenCenter) ||
        !PC->ProjectWorldLocationToScreen(WorldEdge,     ScreenEdge))
    {
        UE_LOG(OcclusionProbeComp, Warning,
            TEXT("UOcclusionProbeComp::CalculateSweepRadius >> Failed to project to screen, returning MinSweepRadius"));
        return MinSweepRadius;
    }

    float PixelRadius   = FVector2D::Distance(ScreenCenter, ScreenEdge);
    float NDCRadius     = PixelRadius / static_cast<float>(ViewportY);

    float DistanceToCamera = FVector::Distance(ActorLocation, CameraManager->GetCameraLocation());
    float HalfFOVRad       = FMath::DegreesToRadians(CameraManager->GetFOVAngle() * 0.5f);
    float FrustumHeight    = 2.f * DistanceToCamera * FMath::Tan(HalfFOVRad);
    float WorldRadius      = NDCRadius * FrustumHeight;

    return FMath::Clamp(WorldRadius, MinSweepRadius, MaxSweepRadius);
}
