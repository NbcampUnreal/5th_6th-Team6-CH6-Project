#include "ObstacleOcclusion/PhysicallOcclusion/OcclusionTracerComponent.h"

#include "ObstacleOcclusion/PhysicallOcclusion/OcclusionTraceLibrary.h"
#include "ObstacleOcclusion/PhysicallOcclusion/FrustumToProjectionMatcherHelper.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "DrawDebugHelpers.h"
#include "TopDownVisionDebug.h"
#include "ObstacleOcclusion/PhysicallOcclusion/OcclusionInterface.h"

UOcclusionTracerComponent::UOcclusionTracerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UOcclusionTracerComponent::BeginPlay()
{
    Super::BeginPlay();

    Probe.Channel = OcclusionChannel;
    Probe.Sweeps  = SweepConfigs;
}

void UOcclusionTracerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    OnBecameHidden();
}

// ── Camera source ─────────────────────────────────────────────────────────────

void UOcclusionTracerComponent::SetCameraComponent(UCameraComponent* InCamera)
{
    if (!ensureMsgf(IsValid(InCamera), TEXT("UOcclusionTracerComponent::SetCameraComponent: null")))
        return;

    CameraComponent = InCamera;
    CameraManager   = nullptr;

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
    GetWorld()->GetTimerManager().ClearTimer(TraceTimerHandle);

    // Clear any active occlusion state so obstacles don't stay faded
    for (const TWeakObjectPtr<AActor>& Previous : Probe.PreviousHits)
    {
        if (Previous.IsValid())
        {
            IOcclusionInterface::Execute_OnOcclusionExit(Previous.Get(), nullptr);
        }
    }

    Probe.PreviousHits.Empty();

    OnTracerDeactivated();

    UE_LOG(Occlusion, Log,
        TEXT("UOcclusionTracerComponent::OnBecameHidden>> Tracer stopped on %s"),
        *GetOwner()->GetName());
}

// ── BP events — default no-op implementations ─────────────────────────────────

void UOcclusionTracerComponent::OnTracerActivated_Implementation()  {}
void UOcclusionTracerComponent::OnTracerDeactivated_Implementation() {}

// ── Internal ──────────────────────────────────────────────────────────────────

void UOcclusionTracerComponent::RunTrace()
{
    if (!IsValid(GetOwner())) return;
    if (!RefreshFrustumParams()) return;

    RebuildProbe();

    FOcclusionTraceLibrary::RunProbe(Probe, GetWorld(), IgnoredActors, bDebugDraw);

    if (bDebugDraw)
        DrawDebug();
}

bool UOcclusionTracerComponent::RefreshFrustumParams()
{
    if (IsValid(CameraComponent))
        return FFrustumProjectionMatcherHelper::ExtractFromCameraComponent(CameraComponent, FrustumParams);

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
    const float   TargetDistance = FVector::Dist(CameraLocation, TargetLocation);

    Probe.BaseOrigin = CameraLocation;
    Probe.Target     = TargetLocation;

    for (int32 i = 0; i < Probe.Sweeps.Num(); ++i)
    {
        const FVector SweepOrigin = CameraLocation + Probe.Sweeps[i].OriginOffset;
        const float   SweepDepth  = FVector::Dist(SweepOrigin, TargetLocation);

        Probe.Sweeps[i].SphereRadius = FFrustumProjectionMatcherHelper::CalculateSphereRadiusAtDepth(
            FrustumParams,
            TargetDistance,
            TargetVisibleRadius,
            SweepDepth);
    }
}

void UOcclusionTracerComponent::DrawDebug() const
{
    UWorld* World = GetWorld();
    if (!World) return;

    const FVector CameraLocation = FrustumParams.CameraLocation;
    const FVector TargetLocation = Probe.Target;

    DrawDebugLine(World, CameraLocation, TargetLocation, DebugColorNoHit, false, -1.f, 0, 1.f);
    DrawDebugSphere(World, TargetLocation, TargetVisibleRadius, 12, DebugColorTarget, false, -1.f);

    for (const FOcclusionSweepConfig& Sweep : Probe.Sweeps)
    {
        const FVector SweepOrigin = CameraLocation + Sweep.OriginOffset;
        DrawDebugSphere(World, SweepOrigin, 4.f, 6, DebugColorSweepOrigin, false, -1.f);
        DrawDebugSphere(World, TargetLocation, Sweep.SphereRadius, 8, DebugColorHit, false, -1.f);
    }
}
/*```

---

The BP usage pattern is now just:
```
BeginPlay         → SetCameraComponent(Camera)
OnBecameVisible   → call OnBecameVisible()   — timer starts
OnBecameHidden    → call OnBecameHidden()    — timer stops, state cleared
OnTracerActivated / OnTracerDeactivated — implement in BP if you need to react*/