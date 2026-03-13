#include "LevelAreaTrackerComponent.h"
#include "Engine/World.h"
#include "LevelManagement/LevelGraphManager/LevelAreaSubsystem/LevelAreaGraphSubsystem.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "LogHelper/DebugLogHelper.h"

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

    UWorld* World = GetWorld();
    if (!World) return;

    ULevelAreaGraphSubsystem* Sub =
        World->GetSubsystem<ULevelAreaGraphSubsystem>();
    if (!Sub) return;

    FVector Start = Owner->GetActorLocation();
    FVector End   = Start - FVector(0.f, 0.f, TraceLength);

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(Owner);
    Params.bReturnPhysicalMaterial = true;

    if (!World->LineTraceSingleByChannel(Hit, Start, End, TraceChannel, Params))
        return;

    int32 NewNodeID = Sub->FindNodeByHitResult(Hit);

    if (NewNodeID != CurrentNodeID)
    {
        int32 OldID = CurrentNodeID;
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
    if (CurrentNodeID == INDEX_NONE) return EAreaHazardState::Safe;

    UWorld* World = GetWorld();
    if (!World) return EAreaHazardState::Safe;

    ULevelAreaGraphSubsystem* Sub =
        World->GetSubsystem<ULevelAreaGraphSubsystem>();
    if (!Sub) return EAreaHazardState::Safe;

    return Sub->GetNodeHazardState(CurrentNodeID);
}

void ULevelAreaTrackerComponent::RefreshHazardState()
{
    EAreaHazardState NewState = GetHazardState();

    if (NewState == CachedHazardState) return;

    EAreaHazardState OldState = CachedHazardState;
    CachedHazardState = NewState;
    CurrentHazardState = NewState;

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("LevelAreaTrackerComponent >> %s hazard state: %s"),
        *GetOwner()->GetName(),
        *UEnum::GetValueAsString(NewState));

    OnHazardStateChanged.Broadcast(OldState, NewState);
}