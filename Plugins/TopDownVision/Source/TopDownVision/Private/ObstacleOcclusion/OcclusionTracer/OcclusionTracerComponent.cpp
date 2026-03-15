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
    Probe.Sweeps  = SweepConfigs;
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

    // PreviousHits now tracks components directly — notify each one individually
    for (const TWeakObjectPtr<UActorComponent>& Previous : Probe.PreviousHits)
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

    // Dispatch to correct method
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

// ── Rebuild probe — virtual so curved world subclass can override ──────────────

void UOcclusionTracerComponent::RebuildProbe()
{
    const FVector TargetLocation = GetOwner()->GetActorLocation() + TargetLocationOffset;
    const FVector CameraLocation = FrustumParams.CameraLocation;
    const FVector LineDirection  = (TargetLocation - CameraLocation).GetSafeNormal();
    const float   LineLength     = FVector::Dist(CameraLocation, TargetLocation);

    Probe.BaseOrigin = CameraLocation;
    Probe.Target     = TargetLocation;

    // Flatten camera forward to XY — used as the depth axis in RunLineProbe.
    // Matches the normal of a vertical plane yaw-rotated toward the camera
    // regardless of camera pitch.
    Probe.CameraForwardH = FVector(
        FrustumParams.CameraForward.X,
        FrustumParams.CameraForward.Y,
        0.f).GetSafeNormal();

    // Secondary sphere filter — radius derived from frustum projection at pawn depth,
    // same method used by sphere array probe to correctly size spheres on screen.
    // Center offset toward camera by half radius so the sphere sits just in front
    // of the pawn rather than centered on it.
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
    else if (bAutoGenerateSweeps)
    {
        GenerateSweepsAlongLine(LineDirection, LineLength);
    }
    else
    {
        for (int32 i = 0; i < Probe.Sweeps.Num(); ++i)
        {
            const FVector SweepOrigin = CameraLocation + Probe.Sweeps[i].OriginOffset;
            const float   SweepDepth  = FVector::Dist(CameraLocation, SweepOrigin);

            Probe.Sweeps[i].SphereRadius = FFrustumProjectionMatcherHelper::CalculateSphereRadiusAtDepth(
                FrustumParams,
                LineLength,
                TargetVisibleRadius,
                SweepDepth);
        }
    }
}

// ── Fibonacci cone generation — virtual for curved world override ─────────────

void UOcclusionTracerComponent::GenerateFibonacciLines(const FVector& TargetLocation)
{
    Probe.Lines.Reset();

    if (NumTracePoints <= 0) return;

    const FVector CameraLocation = FrustumParams.CameraLocation;

    // Build a camera-plane basis so endpoints sit in the plane perpendicular
    // to the camera-to-target direction, centered on the target
    const FVector ToTarget   = (TargetLocation - CameraLocation).GetSafeNormal();

    // Pick a stable up reference — avoid degenerate cross product when ToTarget is nearly vertical
    const FVector WorldUp    = FMath::Abs(ToTarget.Z) < 0.99f ? FVector::UpVector : FVector::RightVector;
    const FVector PlaneRight = FVector::CrossProduct(ToTarget, WorldUp).GetSafeNormal();
    const FVector PlaneUp    = FVector::CrossProduct(PlaneRight, ToTarget).GetSafeNormal();

    // Golden angle in radians — drives the fibonacci spiral
    const float GoldenAngle  = PI * (3.f - FMath::Sqrt(5.f));

    for (int32 i = 0; i < NumTracePoints; ++i)
    {
        // Radius grows outward using sqrt so area density stays even across the disk
        const float Radius = FMath::Sqrt(static_cast<float>(i + 0.5f) / NumTracePoints)
                             * TargetVisibleRadius;

        const float Angle  = i * GoldenAngle;

        // Place endpoint on the plane centered at the target
        const FVector EndPoint = TargetLocation
            + PlaneRight * FMath::Cos(Angle) * Radius
            + PlaneUp    * FMath::Sin(Angle) * Radius;

        FOcclusionLineConfig Line;
        Line.EndPoint = EndPoint;
        Probe.Lines.Add(Line);
    }
}

// ── Sphere array generation — virtual for curved world override ───────────────

void UOcclusionTracerComponent::GenerateSweepsAlongLine(
    const FVector& LineDirection,
    float          LineLength)
{
    Probe.Sweeps.Reset();

    // Start from MinDistFromTarget — skips the area immediately adjacent to the pawn
    // where rocket head shrinkage caused false negatives on low meshes
    float DistToTarget = FMath::Max(MinDistFromTarget, 0.f);

    for (int32 i = 0; i < MaxAutoSweepCount; ++i)
    {
        const float DepthFromCamera = LineLength - DistToTarget;

        if (DepthFromCamera <= 0.f) break;

        // MaxDistFromCamera culling — stop placing spheres beyond this depth
        // Occluders further than this cannot realistically cover the pawn on screen
        if (DepthFromCamera < LineLength - MaxDistFromCamera) break;

        float Radius = 0.f;

        if (RocketHeadDistance > 0.f && DistToTarget <= RocketHeadDistance)
        {
            // Within rocket head zone — 0 at tip, 1 at boundary
            const float NormalizedDist = FMath::Clamp(
                DistToTarget / RocketHeadDistance, 0.f, 1.f);

            // Exp > 1 = concave (sharp tip, gradual widen)
            // Exp < 1 = convex (quick widen near tip)
            // Exp 1   = linear
            Radius = FMath::Pow(NormalizedDist, RocketHeadExponent) * TargetVisibleRadius;

            // Enforce minimum so loop can step forward
            Radius = FMath::Max(Radius, 1.f);
        }
        else
        {
            // Outside rocket head zone — normal frustum projection matching
            Radius = FFrustumProjectionMatcherHelper::CalculateSphereRadiusAtDepth(
                FrustumParams,
                LineLength,
                TargetVisibleRadius,
                DepthFromCamera);

            if (Radius <= KINDA_SMALL_NUMBER) break;
        }

        // Cap radius by remaining distance to target —
        // geometric guarantee that sweep never reaches past the target
        const float ClampedRadius = FMath::Min(Radius, DistToTarget > 0.f ? DistToTarget : 1.f);

        FOcclusionSweepConfig Config;
        Config.OriginOffset = LineDirection * DepthFromCamera;
        Config.SphereRadius = ClampedRadius;
        Probe.Sweeps.Add(Config);

        // Step away from target — gap is current radius * ratio
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

    // Target position — point only, not a sphere
    DrawDebugPoint(World, TargetLocation, 12.f, DebugColorTarget, false, -1.f);

    if (TraceMethod == EOcclusionTraceMethod::FibonacciCone)
    {
        for (int32 i = 0; i < Probe.Lines.Num(); ++i)
        {
            const FVector& EndPoint = Probe.Lines[i].EndPoint;

            // Orange = this line hit a valid occlusion comp, Green = no hit
            const FColor LineColor = Probe.HitLineIndices.Contains(i)
                ? FColor::Orange
                : DebugColorNoHit;

            DrawDebugLine(World, CameraLocation, EndPoint, LineColor, false, -1.f, 0, 0.5f);
            DrawDebugPoint(World, EndPoint, 6.f, DebugColorSweepOrigin, false, -1.f);
        }

        // Show secondary filter sphere — helps tune radius visually
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
            const FOcclusionSweepConfig& Sweep       = Probe.Sweeps[i];
            const FVector                SweepOrigin = CameraLocation + Sweep.OriginOffset;

            // Orange = this sweep hit a valid occlusion comp this frame, Red = no hit
            const FColor SphereColor = Probe.HitSweepIndices.Contains(i)
                ? FColor::Orange
                : DebugColorHit;

            DrawDebugSphere(World, SweepOrigin, 4.f, 6, DebugColorSweepOrigin, false, -1.f);
            DrawDebugSphere(World, SweepOrigin, Sweep.SphereRadius, 8, SphereColor, false, -1.f);
        }

        // Show rocket head zone boundary — magenta line from target to where taper ends.
        // If MinDistFromTarget >= RocketHeadDistance the zone is skipped entirely
        // and no rocket head spheres will appear — tune accordingly.
        if (RocketHeadDistance > 0.f)
        {
            const FVector LineDir       = (TargetLocation - CameraLocation).GetSafeNormal();
            const FVector RocketHeadEnd = TargetLocation + LineDir * RocketHeadDistance;

            // Magenta line shows full rocket head zone extent
            DrawDebugLine(World, TargetLocation, RocketHeadEnd, FColor::Magenta, false, -1.f, 0, 1.5f);

            // Magenta sphere marks where taper ends and normal frustum sizing begins
            DrawDebugSphere(World, RocketHeadEnd, 8.f, 8, FColor::Magenta, false, -1.f);
        }

        // White sphere shows where the first sweep actually starts (MinDistFromTarget).
        // If this sits further from target than the magenta sphere, rocket head is skipped.
        if (MinDistFromTarget > 0.f)
        {
            const FVector LineDir        = (TargetLocation - CameraLocation).GetSafeNormal();
            const FVector FirstSweepPos  = TargetLocation + LineDir * MinDistFromTarget;

            DrawDebugSphere(World, FirstSweepPos, 8.f, 8, FColor::White, false, -1.f);
        }

        // MaxDistFromCamera boundary — shows where sphere placement stops on the camera side
        {
            const FVector LineDir         = (TargetLocation - CameraLocation).GetSafeNormal();
            const float   ActualLineLength = FVector::Dist(CameraLocation, TargetLocation);
            const float   CullDepth       = ActualLineLength - MaxDistFromCamera;

            if (CullDepth > 0.f)
            {
                const FVector CullPos = CameraLocation + LineDir * MaxDistFromCamera;

                // Blue sphere marks the far culling boundary
                DrawDebugSphere(World, CullPos, 8.f, 8, FColor::Blue, false, -1.f);
            }
        }
    }
}