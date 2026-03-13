#include "TopDownVision/Public/ObstacleOcclusion/PhysicalOcclusion/OcclusionTracerComponent.h"

#include "TopDownVision/Public/ObstacleOcclusion/PhysicalOcclusion/OcclusionTraceLibrary.h"
#include "TopDownVision/Public/ObstacleOcclusion/PhysicalOcclusion/FrustumToProjectionMatcherHelper.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "DrawDebugHelpers.h"
#include "TopDownVisionDebug.h"
#include "TopDownVision/Public/ObstacleOcclusion/OcclusionInterface.h"
#include "TopDownVision/Public/ObstacleOcclusion/PhysicalOcclusion/OcclusionObstacleComp_Physical.h"

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
    OnBecameHidden();
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

void UOcclusionTracerComponent::OnBecameVisible()
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

void UOcclusionTracerComponent::OnBecameHidden()
{
    if (!IsActive()) return;

    GetWorld()->GetTimerManager().ClearTimer(TraceTimerHandle);

    for (const TWeakObjectPtr<AActor>& Previous : Probe.PreviousHits)
    {
        if (!Previous.IsValid()) continue;

        TArray<UOcclusionObstacleComp_Physical*> Comps;
        Previous->GetComponents<UOcclusionObstacleComp_Physical>(Comps);

        for (UOcclusionObstacleComp_Physical* Comp : Comps)
        {
            IOcclusionInterface::Execute_OnOcclusionExit(Comp, this);
        }
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

    FOcclusionTraceLibrary::RunProbe(Probe, GetWorld(), IgnoredActors, this, bDebugDraw);

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

void UOcclusionTracerComponent::RebuildProbe()
{
    const FVector TargetLocation = GetOwner()->GetActorLocation() + TargetLocationOffset;
    const FVector CameraLocation = FrustumParams.CameraLocation;
    const FVector LineDirection  = (TargetLocation - CameraLocation).GetSafeNormal();
    const float   LineLength     = FVector::Dist(CameraLocation, TargetLocation);

    Probe.BaseOrigin = CameraLocation;
    Probe.Target     = TargetLocation;

    if (bAutoGenerateSweeps)
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

void UOcclusionTracerComponent::GenerateSweepsAlongLine(
    const FVector& LineDirection,
    float          LineLength)
{
    Probe.Sweeps.Reset();

    // Start exactly at target, walk toward camera
    float DistToTarget = 0.f;

    for (int32 i = 0; i < MaxAutoSweepCount; ++i)
    {
        const float DepthFromCamera = LineLength - DistToTarget;

        if (DepthFromCamera <= 0.f) break;

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

            // Enforce minimum so loop can step forward — skip tiny spheres at the very tip
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

void UOcclusionTracerComponent::DrawDebug() const
{
    UWorld* World = GetWorld();
    if (!World) return;

    const FVector CameraLocation = FrustumParams.CameraLocation;
    const FVector TargetLocation = Probe.Target;

    DrawDebugLine(World, CameraLocation, TargetLocation, DebugColorNoHit, false, -1.f, 5, 0.5f);

    // Target position — point only, not a sphere, to avoid implying it is a trace shape
    DrawDebugPoint(World, TargetLocation, 12.f, DebugColorTarget, false, -1.f);

    for (int32 i = 0; i < Probe.Sweeps.Num(); ++i)
    {
        const FOcclusionSweepConfig& Sweep      = Probe.Sweeps[i];
        const FVector                SweepOrigin = CameraLocation + Sweep.OriginOffset;

        // Orange = this sweep hit a valid occlusion comp this frame, Red = no hit
        const FColor SphereColor = Probe.HitSweepIndices.Contains(i)
            ? FColor::Orange
            : DebugColorHit;

        DrawDebugSphere(World, SweepOrigin, 4.f, 6, DebugColorSweepOrigin, false, -1.f);
        DrawDebugSphere(World, SweepOrigin, Sweep.SphereRadius, 8, SphereColor, false, -1.f);
    }
}