#include "LevelAreaTrackerComponent.h"
#include "Engine/World.h"
#include "LevelManagement/LevelGraphManager/LevelAreaSubsystem/LevelAreaGraphSubsystem.h"
#include "LogHelper/DebugLogHelper.h"
#include "Requirements/LevelAreaPhysicalMaterial.h"

ULevelAreaTrackerComponent::ULevelAreaTrackerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void ULevelAreaTrackerComponent::BeginPlay()
{
    Super::BeginPlay();
    UpdateArea();
}


void ULevelAreaTrackerComponent::UpdateArea()
{
    AActor* Owner = GetOwner();
    if (!Owner) return;

    int32 NewNodeID = TraceForNodeID();

    if (NewNodeID != CurrentNodeID)
    {
        int32 OldID   = CurrentNodeID;
        CurrentNodeID = NewNodeID;

        UE_LOG(LevelAreaGraphManagement, Log,
            TEXT("LevelAreaTrackerComponent >> %s moved from Node %d to Node %d"),
            *Owner->GetName(), OldID, CurrentNodeID);

        OnNodeChanged.Broadcast(OldID, CurrentNodeID);
    }

    RefreshHazardState();
}

EAreaHazardState ULevelAreaTrackerComponent::GetHazardState() const
{
    return GetHazardStateForNode(CurrentNodeID);
}

void ULevelAreaTrackerComponent::RefreshHazardState()
{
    EAreaHazardState NewState = GetHazardState();
    if (NewState == CachedHazardState) return;

    EAreaHazardState OldState = CachedHazardState;
    CachedHazardState  = NewState;
    CurrentHazardState = NewState;

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("LevelAreaTrackerComponent >> %s hazard state: %s"),
        *GetOwner()->GetName(), *UEnum::GetValueAsString(NewState));

    OnHazardStateChanged.Broadcast(OldState, NewState);
}


/* =====================================================================
   Debug
   ===================================================================== */

int32 ULevelAreaTrackerComponent::TraceForNodeID() const
{
    const AActor* Owner = GetOwner();
    if (!Owner) return INDEX_NONE;

    const UWorld* World = GetWorld();
    if (!World) return INDEX_NONE;

    FVector Start = Owner->GetActorLocation();
    FVector End   = Start - FVector(0.f, 0.f, TraceLength);

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(Owner);
    Params.bReturnPhysicalMaterial = true;

    if (!World->LineTraceSingleByChannel(Hit, Start, End, TraceChannel, Params))
        return INDEX_NONE;

    if (ULevelAreaPhysicalMaterial* AreaMat =
        Cast<ULevelAreaPhysicalMaterial>(Hit.PhysMaterial.Get()))
    {
        return AreaMat->NodeID;
    }

    return INDEX_NONE;
}

EAreaHazardState ULevelAreaTrackerComponent::GetHazardStateForNode(int32 InNodeID) const
{
    if (InNodeID == INDEX_NONE) return EAreaHazardState::Safe;

    const UWorld* World = GetWorld();
    if (!World) return EAreaHazardState::Safe;

    const ULevelAreaGraphSubsystem* Sub =
        World->GetSubsystem<ULevelAreaGraphSubsystem>();
    if (!Sub) return EAreaHazardState::Safe;

    return Sub->GetNodeHazardState(InNodeID);
}