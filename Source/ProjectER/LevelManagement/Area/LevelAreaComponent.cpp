#include "LevelAreaComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "LevelManagement/LevelAreaTrackerComponent.h"
#include "LevelManagement/LevelGraphManager/LevelAreaSubsystem/LevelAreaGraphSubsystem.h"
#include "LogHelper/DebugLogHelper.h"

ULevelAreaComponent::ULevelAreaComponent()
{
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = false;
}

void ULevelAreaComponent::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ULevelAreaComponent, bIsHazard);
}

void ULevelAreaComponent::BeginPlay()
{
    Super::BeginPlay();

    // Only the server builds and registers the graph
    if (GetOwner()->HasAuthority())
    {
        RegisterWithSubsystem();
    }
}

void ULevelAreaComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (GetOwner()->HasAuthority())
    {
        UnregisterFromSubsystem();
    }
    Super::EndPlay(EndPlayReason);
}

void ULevelAreaComponent::RegisterWithSubsystem()
{
    if (NodeID == INDEX_NONE)
    {
        UE_LOG(LevelAreaGraphManagement, Error,
            TEXT("LevelAreaComponent on %s >> NodeID not set"),
            *GetOwner()->GetName());
        return;
    }

    OwnedNode = MakeUnique<LevelAreaNode>();
    OwnedNode->SetNodeID(NodeID);
    OwnedNode->SetRoomCenter(RoomCenter);
    OwnedNode->SetRoomExtent(RoomExtent);

    // Connections are wired here; bridge geometry defaults to room centers
    // (override BridgeStart/End per connection if you have precise portal data)
    for (int32 NeighborID : NeighborNodeIDs)
    {
        OwnedNode->AddConnection(NeighborID, RoomCenter, RoomCenter);
    }

    if (ULevelAreaGraphSubsystem* Sub =
        GetWorld()->GetSubsystem<ULevelAreaGraphSubsystem>())
    {
        Sub->RegisterNode(OwnedNode.Get());
    }
}

void ULevelAreaComponent::UnregisterFromSubsystem()
{
    if (!OwnedNode) return;

    if (ULevelAreaGraphSubsystem* Sub =
        GetWorld()->GetSubsystem<ULevelAreaGraphSubsystem>())
    {
        Sub->UnregisterNode(OwnedNode->GetNodeID());
    }

    OwnedNode.Reset();
}

void ULevelAreaComponent::SetHazard(bool bNewHazard)
{
    // Authority only — bIsHazard replicates to clients automatically
    if (!GetOwner()->HasAuthority()) return;

    bIsHazard = bNewHazard;
    OnHazardStateChanged.Broadcast(bIsHazard);
}

void ULevelAreaComponent::OnRep_bIsHazard()
{
    // Clients react here: trigger VFX, audio, UI highlights, etc.
    OnHazardStateChanged.Broadcast(bIsHazard);

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("LevelAreaComponent >> Node %d hazard state: %s"),
        NodeID, bIsHazard ? TEXT("HAZARD") : TEXT("SAFE"));
}

void ULevelAreaComponent::OnPawnEntered(APawn* Pawn)
{
    if (!Pawn) return;
    PawnsInside.AddUnique(Pawn);

    if (ULevelAreaTrackerComponent* Tracker =
        Pawn->FindComponentByClass<ULevelAreaTrackerComponent>())
    {
        Tracker->NotifyEnterNode(NodeID);
    }
}

void ULevelAreaComponent::OnPawnExited(APawn* Pawn)
{
    PawnsInside.Remove(Pawn);

    if (ULevelAreaTrackerComponent* Tracker =
        Pawn->FindComponentByClass<ULevelAreaTrackerComponent>())
    {
        Tracker->NotifyExitNode(NodeID);
    }
}