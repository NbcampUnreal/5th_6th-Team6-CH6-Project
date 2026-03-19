#include "ObstacleOcclusion/Usage/Roof/BuildingTrackerComponent.h"
#include "ObstacleOcclusion/Usage/Roof/BuildingOcclusionManager.h"
#include "TopDownVisionDebug.h"

DEFINE_LOG_CATEGORY(BuildingTracker);

UBuildingTrackerComponent::UBuildingTrackerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UBuildingTrackerComponent::BeginPlay()
{
    Super::BeginPlay();
}

// ── API ───────────────────────────────────────────────────────────────────────

void UBuildingTrackerComponent::SetTrackerActivation(bool bActivate)
{
    bInitialized = bActivate;

    if (!bActivate && IsTrackerLoopActive())
        StopTrackerLoop();

    UE_LOG(BuildingTracker, Log,
        TEXT("UBuildingTrackerComponent::SetTrackerActivation>> %s | %s"),
        *GetOwner()->GetName(),
        bActivate ? TEXT("activated") : TEXT("deactivated"));
}

void UBuildingTrackerComponent::StartTrackerLoop()
{
    UWorld* World = GetWorld();
    if (!World) return;

    if (World->GetTimerManager().IsTimerActive(TrackerTimerHandle))
    {
        return;
    }

    World->GetTimerManager().SetTimer(
        TrackerTimerHandle,
        this,
        &UBuildingTrackerComponent::UpdateBuildingState,
        TraceInterval,
        true);
}

void UBuildingTrackerComponent::StopTrackerLoop()
{
    UWorld* World = GetWorld();
    if (!World) return;

    if (!World->GetTimerManager().IsTimerActive(TrackerTimerHandle))
    {
        return;
    }

    World->GetTimerManager().ClearTimer(TrackerTimerHandle);

    // One final update to lock state to where the pawn landed
    UpdateBuildingState();
}

bool UBuildingTrackerComponent::IsTrackerLoopActive() const
{
    const UWorld* World = GetWorld();
    if (!World) return false;

    return World->GetTimerManager().IsTimerActive(TrackerTimerHandle);
}

// ── Internal ──────────────────────────────────────────────────────────────────

void UBuildingTrackerComponent::UpdateBuildingState()
{
    AActor* Owner = GetOwner();
    if (!IsValid(Owner)) return;

    UWorld* World = GetWorld();
    if (!World) return;

    const FVector Start = Owner->GetActorLocation();
    const FVector End   = Start - FVector(0.f, 0.f, TraceLength);

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(Owner);

    const bool bHit = World->LineTraceSingleByChannel(
        Hit, Start, End, InteriorTraceChannel, Params);

    AActor* NewProxyActor = bHit ? Hit.GetActor() : nullptr;

    // No area change
    if (NewProxyActor == LastHitProxyActor.Get()) return;

    AActor* OldProxyActor = LastHitProxyActor.Get();

    ABuildingOcclusionManager* OldManager = nullptr;
    if (IsValid(OldProxyActor))
        OldManager = Cast<ABuildingOcclusionManager>(OldProxyActor->GetAttachParentActor());

    ABuildingOcclusionManager* NewManager = nullptr;
    if (IsValid(NewProxyActor))
    {
        NewManager = Cast<ABuildingOcclusionManager>(NewProxyActor->GetAttachParentActor());

        if (!NewManager)
        {
            UE_LOG(BuildingTracker, Warning,
                TEXT("UBuildingTrackerComponent::UpdateBuildingState>> %s hit %s but no manager found as attach parent"),
                *Owner->GetName(), *NewProxyActor->GetName());
        }
    }

    LastHitProxyActor = NewProxyActor;

    const bool bWasInArea = IsValid(OldProxyActor);
    const bool bNowInArea = IsValid(NewProxyActor);

    if (!bWasInArea && bNowInArea)
    {
        // Outside → room
        UE_LOG(BuildingTracker, Log,
            TEXT("UBuildingTrackerComponent::UpdateBuildingState>> %s outside → room"),
            *Owner->GetName());
        OnEnteredRoom(NewManager);
    }
    else if (bWasInArea && !bNowInArea)
    {
        // Room → outside
        UE_LOG(BuildingTracker, Log,
            TEXT("UBuildingTrackerComponent::UpdateBuildingState>> %s room → outside"),
            *Owner->GetName());
        OnExitedRoom(OldManager);
    }
    else if (bWasInArea && bNowInArea)
    {
        // Room A → room B
        UE_LOG(BuildingTracker, Log,
            TEXT("UBuildingTrackerComponent::UpdateBuildingState>> %s room A → room B"),
            *Owner->GetName());
        OnExitedRoom(OldManager);
        OnEnteredRoom(NewManager);
    }
}

void UBuildingTrackerComponent::OnEnteredRoom(ABuildingOcclusionManager* Manager) const
{
    if (!IsValid(Manager)) return;

    Manager->OnInteriorEnter();

    UE_LOG(BuildingTracker, Log,
        TEXT("UBuildingTrackerComponent::OnEnteredRoom>> %s entered %s"),
        *GetOwner()->GetName(), *Manager->GetName());
}

void UBuildingTrackerComponent::OnExitedRoom(ABuildingOcclusionManager* Manager) const
{
    if (!IsValid(Manager)) return;

    Manager->OnInteriorExit();

    UE_LOG(BuildingTracker, Log,
        TEXT("UBuildingTrackerComponent::OnExitedRoom>> %s exited %s"),
        *GetOwner()->GetName(), *Manager->GetName());
}