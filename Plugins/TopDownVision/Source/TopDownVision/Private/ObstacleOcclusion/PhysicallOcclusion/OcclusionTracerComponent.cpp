#include "ObstacleOcclusion/PhysicallOcclusion/OcclusionTracerComponent.h"

#include "ObstacleOcclusion/PhysicallOcclusion/OcclusionTraceLibrary.h"
#include "ObstacleOcclusion/PhysicallOcclusion/FrustumToProjectionMatcherHelper.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "DrawDebugHelpers.h"
#include "TopDownVisionDebug.h"
#include "ObstacleOcclusion/PhysicallOcclusion/OcclusionInterface.h"
#include "ObstacleOcclusion/PhysicallOcclusion/OcclusionObstacleComponent.h"

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

        TArray<UOcclusionObstacleComponent*> Comps;
        Previous->GetComponents<UOcclusionObstacleComponent>(Comps);

        for (UOcclusionObstacleComponent* Comp : Comps)
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
        // Radii computed inside GenerateSweepsAlongLine
        GenerateSweepsAlongLine(LineDirection, LineLength);
    }
    else
    {
        // Manual config — compute radii from depth
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

    // Start one TargetVisibleRadius away from the target, walk toward camera
    float DepthFromCamera = LineLength - TargetVisibleRadius;

    for (int32 i = 0; i < MaxAutoSweepCount; ++i)
    {
        if (DepthFromCamera <= 0.f) break;

        const float Radius = FFrustumProjectionMatcherHelper::CalculateSphereRadiusAtDepth(
            FrustumParams,
            LineLength,
            TargetVisibleRadius,
            DepthFromCamera);

        if (Radius <= KINDA_SMALL_NUMBER) break;

        FOcclusionSweepConfig Config;
        Config.OriginOffset = LineDirection * DepthFromCamera;
        Config.SphereRadius = Radius;
        Probe.Sweeps.Add(Config);

        // Step toward camera — gap is current radius * ratio
        DepthFromCamera -= Radius * SweepGapRatio;
    }
}

void UOcclusionTracerComponent::DrawDebug() const
{
    UWorld* World = GetWorld();
    if (!World) return;

    const FVector CameraLocation = FrustumParams.CameraLocation;
    const FVector TargetLocation = Probe.Target;

    DrawDebugLine(World, CameraLocation, TargetLocation, DebugColorNoHit, false, -1.f, 5, 0.5f);
    DrawDebugSphere(World, TargetLocation, TargetVisibleRadius, 12, DebugColorTarget, false, -1.f);

    for (const FOcclusionSweepConfig& Sweep : Probe.Sweeps)
    {
        const FVector SweepOrigin = CameraLocation + Sweep.OriginOffset;

        DrawDebugSphere(World, SweepOrigin, 4.f, 6, DebugColorSweepOrigin, false, -1.f);
        DrawDebugSphere(World, SweepOrigin, Sweep.SphereRadius, 8, DebugColorHit, false, -1.f);
    }
}