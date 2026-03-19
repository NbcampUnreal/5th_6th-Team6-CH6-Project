#include "TopDownVision/Public/ObstacleOcclusion/OcclusionTracer/OcclusionTracerComponent.h"

#include "TopDownVision/Public/ObstacleOcclusion/OcclusionTracer/OcclusionTraceLibrary.h"
#include "TopDownVision/Public/ObstacleOcclusion/PhysicalOcclusion/FrustumToProjectionMatcherHelper.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "DrawDebugHelpers.h"
#include "TopDownVisionDebug.h"
#include "TopDownVision/Public/ObstacleOcclusion/OcclusionTracer/OcclusionInterface.h"

UOcclusionTracerComponent::UOcclusionTracerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UOcclusionTracerComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UOcclusionTracerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    OnOwnerBecameHidden();
}

void UOcclusionTracerComponent::InitializeProbeTracer()
{
    if (GetNetMode() == NM_DedicatedServer)
    {
        Deactivate();
        return;
    }

    Activate();
    Probe.Channel = OcclusionChannel;
}

// ── Camera source ─────────────────────────────────────────────────────────────

void UOcclusionTracerComponent::SetCameraComponent(UCameraComponent* InCamera)
{
    if (!ensureMsgf(IsValid(InCamera), TEXT("UOcclusionTracerComponent::SetCameraComponent: null")))
        return;

    CameraComponent = InCamera;

    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
        CameraManager = PC->PlayerCameraManager;

    IgnoredActors.Reset();
    IgnoredActors.Add(InCamera->GetOwner());
    IgnoredActors.Add(GetOwner());

    bCameraReady = true;

    UE_LOG(Occlusion, Log,
        TEXT("UOcclusionTracerComponent::SetCameraComponent>> Set on %s"),
        *GetOwner()->GetName());
}

void UOcclusionTracerComponent::SetCameraManager(APlayerCameraManager* InCameraManager)
{
    if (!ensureMsgf(IsValid(InCameraManager), TEXT("UOcclusionTracerComponent::SetCameraManager: null")))
        return;

    CameraManager   = InCameraManager;
    CameraComponent = nullptr;

    IgnoredActors.Reset();
    IgnoredActors.Add(GetOwner());

    bCameraReady = true;

    UE_LOG(Occlusion, Log,
        TEXT("UOcclusionTracerComponent::SetCameraManager>> Set on %s"),
        *GetOwner()->GetName());
}

// ── Visibility API ────────────────────────────────────────────────────────────

void UOcclusionTracerComponent::OnOwnerBecameVisible()
{
    if (!IsActive()) return;

    if (!bCameraReady)
    {
        UE_LOG(Occlusion, Warning,
            TEXT("UOcclusionTracerComponent::OnBecameVisible>> No camera source set on %s"),
            *GetOwner()->GetName());
        return;
    }

    if (!GetWorld()->GetTimerManager().IsTimerActive(TraceTimerHandle))
    {
        GetWorld()->GetTimerManager().SetTimer(
            TraceTimerHandle,
            this,
            &UOcclusionTracerComponent::RunTrace,
            TraceInterval,
            true);
    }

    OnTracerActivated();

    UE_LOG(Occlusion, Log,
        TEXT("UOcclusionTracerComponent::OnBecameVisible>> Tracer started on %s"),
        *GetOwner()->GetName());
}

void UOcclusionTracerComponent::OnOwnerBecameHidden()
{
    if (!IsActive()) return;

    GetWorld()->GetTimerManager().ClearTimer(TraceTimerHandle);

    for (const TWeakObjectPtr<UObject>& Previous : Probe.PreviousHits)
    {
        if (!Previous.IsValid()) continue;
        IOcclusionInterface::Execute_OnOcclusionExit(Previous.Get(), this);
    }

    Probe.PreviousHits.Empty();

    OnTracerDeactivated();

    UE_LOG(Occlusion, Log,
        TEXT("UOcclusionTracerComponent::OnBecameHidden>> Tracer stopped on %s"),
        *GetOwner()->GetName());
}

void UOcclusionTracerComponent::OnTracerActivated_Implementation()  {}
void UOcclusionTracerComponent::OnTracerDeactivated_Implementation() {}

// ── Internal ──────────────────────────────────────────────────────────────────

void UOcclusionTracerComponent::RunTrace()
{
    if (!IsValid(GetOwner())) return;
    if (!RefreshFrustumParams()) return;

    RebuildProbe();

    if (TraceMethod == EOcclusionTraceMethod::FibonacciCone)
    {
        FOcclusionTraceLibrary::RunLineProbe(Probe, GetWorld(), IgnoredActors, this, bDebugDraw);
    }
    else
    {
        FOcclusionTraceLibrary::RunProbe(Probe, GetWorld(), IgnoredActors, this, bDebugDraw);
    }

    if (bDebugDraw)
        DrawDebug();
}

bool UOcclusionTracerComponent::RefreshFrustumParams()
{
    if (IsValid(CameraComponent))
    {
        if (!FFrustumProjectionMatcherHelper::ExtractFromCameraComponent(CameraComponent, FrustumParams))
            return false;

        if (IsValid(CameraManager))
        {
            FrustumParams.CameraLocation = CameraManager->GetCameraLocation();
            FrustumParams.CameraForward  = CameraManager->GetCameraRotation().Vector();
        }

        return true;
    }

    if (IsValid(CameraManager))
        return FFrustumProjectionMatcherHelper::ExtractFromCameraManager(CameraManager, FrustumParams);

    UE_LOG(Occlusion, Warning,
        TEXT("UOcclusionTracerComponent::RefreshFrustumParams>> No valid camera source on %s"),
        *GetOwner()->GetName());

    return false;
}

// ── Rebuild probe ─────────────────────────────────────────────────────────────

void UOcclusionTracerComponent::RebuildProbe()
{
    const FVector TargetLocation = GetOwner()->GetActorLocation() + TargetLocationOffset;
    const FVector CameraLocation = FrustumParams.CameraLocation;
    const FVector LineDirection  = (TargetLocation - CameraLocation).GetSafeNormal();
    const float   LineLength     = FVector::Dist(CameraLocation, TargetLocation);

    Probe.BaseOrigin = CameraLocation;
    Probe.Target     = TargetLocation;

    Probe.CameraForwardH = FVector(
        FrustumParams.CameraForward.X,
        FrustumParams.CameraForward.Y,
        0.f).GetSafeNormal();

    const float FrustumRadius = FFrustumProjectionMatcherHelper::CalculateSphereRadiusAtDepth(
        FrustumParams,
        LineLength,
        TargetVisibleRadius,
        LineLength);

    const FVector ToCameraDir = (CameraLocation - TargetLocation).GetSafeNormal();
    Probe.SecondaryFilterSphereCenter = TargetLocation
        + ToCameraDir * (FrustumRadius * 0.5f);
    Probe.SecondaryFilterSphereRadius = FrustumRadius;

    if (TraceMethod == EOcclusionTraceMethod::FibonacciCone)
    {
        GenerateFibonacciLines(TargetLocation);
    }
    else
    {
        // Sphere array always auto-generates
        GenerateSweepsAlongLine(LineDirection, LineLength);
    }
}

// ── Fibonacci cone generation ─────────────────────────────────────────────────

void UOcclusionTracerComponent::GenerateFibonacciLines(const FVector& TargetLocation)
{
    Probe.Lines.Reset();

    if (NumTracePoints <= 0) return;

    const FVector CameraLocation = FrustumParams.CameraLocation;
    const FVector ToTarget       = (TargetLocation - CameraLocation).GetSafeNormal();
    const FVector WorldUp        = FMath::Abs(ToTarget.Z) < 0.99f ? FVector::UpVector : FVector::RightVector;
    const FVector PlaneRight     = FVector::CrossProduct(ToTarget, WorldUp).GetSafeNormal();
    const FVector PlaneUp        = FVector::CrossProduct(PlaneRight, ToTarget).GetSafeNormal();
    const float   GoldenAngle    = PI * (3.f - FMath::Sqrt(5.f));

    for (int32 i = 0; i < NumTracePoints; ++i)
    {
        const float Radius = FMath::Sqrt(static_cast<float>(i + 0.5f) / NumTracePoints)
                             * TargetVisibleRadius;
        const float Angle  = i * GoldenAngle;

        const FVector EndPoint = TargetLocation
            + PlaneRight * FMath::Cos(Angle) * Radius
            + PlaneUp    * FMath::Sin(Angle) * Radius;

        FOcclusionLineConfig Line;
        Line.EndPoint = EndPoint;
        Probe.Lines.Add(Line);
    }
}

// ── Sphere array generation ───────────────────────────────────────────────────

void UOcclusionTracerComponent::GenerateSweepsAlongLine(
    const FVector& LineDirection,
    float          LineLength)
{
    Probe.Sweeps.Reset();

    float DistToTarget     = FMath::Max(MinDistFromTarget, 0.f);
    float PrevNearEdge     = DistToTarget; // near edge of previous sphere toward target
    float PrevRadius       = 0.f;

    for (int32 i = 0; i < MaxAutoSweepCount; ++i)
    {
        const float DepthFromCamera = LineLength - DistToTarget;

        if (DepthFromCamera <= 0.f) break;
        if (DepthFromCamera < LineLength - MaxDistFromCamera) break;

        float Radius = 0.f;

        if (RocketHeadDistance > 0.f && DistToTarget <= RocketHeadDistance)
        {
            // Within rocket head zone — taper from 0 at tip to TargetVisibleRadius at boundary
            const float NormalizedDist = FMath::Clamp(
                DistToTarget / RocketHeadDistance, 0.f, 1.f);

            Radius = FMath::Pow(NormalizedDist, RocketHeadExponent) * TargetVisibleRadius;
            Radius = FMath::Max(Radius, 1.f);
        }
        else
        {
            Radius = FFrustumProjectionMatcherHelper::CalculateSphereRadiusAtDepth(
                FrustumParams,
                LineLength,
                TargetVisibleRadius,
                DepthFromCamera);

            if (Radius <= KINDA_SMALL_NUMBER) break;
        }

        // Cap 1 — never extend past the target
        Radius = FMath::Min(Radius, DistToTarget > 0.f ? DistToTarget : 1.f);

        // Cap 2 — never overlap backward into the previous sphere's area
        // Each sphere's near edge (toward target) must not go past the previous sphere's near edge
        // Near edge of this sphere = DistToTarget - Radius
        // Must be >= PrevNearEdge, so Radius <= DistToTarget - PrevNearEdge
        if (i > 0)
        {
            const float MaxNonOverlapRadius = DistToTarget - PrevNearEdge;
            Radius = FMath::Min(Radius, FMath::Max(MaxNonOverlapRadius, 1.f));
        }

        FOcclusionSweepConfig Config;
        Config.OriginOffset = LineDirection * DepthFromCamera;
        Config.SphereRadius = Radius;
        Probe.Sweeps.Add(Config);

        // Store near edge for next iteration
        PrevNearEdge = DistToTarget - Radius;
        PrevRadius   = Radius;

        // Step away from target
        DistToTarget += Radius * SweepGapRatio;
    }
}

// ── Debug ─────────────────────────────────────────────────────────────────────

void UOcclusionTracerComponent::DrawDebug() const
{
    UWorld* World = GetWorld();
    if (!World) return;

    const FVector CameraLocation = FrustumParams.CameraLocation;
    const FVector TargetLocation = Probe.Target;

    DrawDebugLine(World, CameraLocation, TargetLocation, DebugColorNoHit, false, -1.f, 5, 0.5f);
    DrawDebugPoint(World, TargetLocation, 12.f, DebugColorTarget, false, -1.f);

    if (TraceMethod == EOcclusionTraceMethod::FibonacciCone)
    {
        for (int32 i = 0; i < Probe.Lines.Num(); ++i)
        {
            const FVector& EndPoint = Probe.Lines[i].EndPoint;

            const FColor LineColor = Probe.HitLineIndices.Contains(i)
                ? FColor::Orange
                : DebugColorNoHit;

            DrawDebugLine(World, CameraLocation, EndPoint, LineColor, false, -1.f, 0, 0.5f);
            DrawDebugPoint(World, EndPoint, 6.f, DebugColorSweepOrigin, false, -1.f);
        }

        if (Probe.SecondaryFilterSphereRadius > 0.f)
        {
            DrawDebugSphere(
                World,
                Probe.SecondaryFilterSphereCenter,
                Probe.SecondaryFilterSphereRadius,
                16,
                FColor::Cyan,
                false,
                -1.f);
        }
    }
    else
    {
        for (int32 i = 0; i < Probe.Sweeps.Num(); ++i)
        {
            const FOcclusionSweepConfig& Sweep      = Probe.Sweeps[i];
            const FVector                SweepOrigin = CameraLocation + Sweep.OriginOffset;

            const FColor SphereColor = Probe.HitSweepIndices.Contains(i)
                ? FColor::Orange
                : DebugColorHit;

            DrawDebugSphere(World, SweepOrigin, 4.f, 6, DebugColorSweepOrigin, false, -1.f);
            DrawDebugSphere(World, SweepOrigin, Sweep.SphereRadius, 8, SphereColor, false, -1.f);
        }

        if (RocketHeadDistance > 0.f)
        {
            const FVector LineDir       = (TargetLocation - CameraLocation).GetSafeNormal();
            const FVector RocketHeadEnd = TargetLocation + LineDir * RocketHeadDistance;

            DrawDebugLine(World, TargetLocation, RocketHeadEnd, FColor::Magenta, false, -1.f, 0, 1.5f);
            DrawDebugSphere(World, RocketHeadEnd, 8.f, 8, FColor::Magenta, false, -1.f);
        }

        if (MinDistFromTarget > 0.f)
        {
            const FVector LineDir       = (TargetLocation - CameraLocation).GetSafeNormal();
            const FVector FirstSweepPos = TargetLocation + LineDir * MinDistFromTarget;

            DrawDebugSphere(World, FirstSweepPos, 8.f, 8, FColor::White, false, -1.f);
        }

        {
            const FVector LineDir         = (TargetLocation - CameraLocation).GetSafeNormal();
            const float   ActualLineLength = FVector::Dist(CameraLocation, TargetLocation);
            const float   CullDepth       = ActualLineLength - MaxDistFromCamera;

            if (CullDepth > 0.f)
            {
                const FVector CullPos = CameraLocation + LineDir * MaxDistFromCamera;
                DrawDebugSphere(World, CullPos, 8.f, 8, FColor::Blue, false, -1.f);
            }
        }
    }
}