#include "LevelAreaActor.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "LevelManagement/LevelGraphManager/LevelAreaSubsystem/LevelAreaGraphSubsystem.h"
#include "LogHelper/DebugLogHelper.h"

ALevelAreaActor::ALevelAreaActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    // Bare root — designer attaches whatever meshes they want
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void ALevelAreaActor::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ALevelAreaActor, HazardState);
}


/* =====================================================================
   Editor Utility
   ===================================================================== */

void ALevelAreaActor::ApplyMaterialToMeshes()
{
    if (!FloorMaterial)
    {
        UE_LOG(LevelAreaGraphManagement, Warning,
            TEXT("ALevelAreaActor >> ApplyMaterialToMeshes: FloorMaterial is not set on %s"),
            *GetName());
        return;
    }

    TArray<UStaticMeshComponent*> MeshComponents;
    GetComponents<UStaticMeshComponent>(MeshComponents);

    if (MeshComponents.IsEmpty())
    {
        UE_LOG(LevelAreaGraphManagement, Warning,
            TEXT("ALevelAreaActor >> ApplyMaterialToMeshes: No StaticMeshComponents found on %s"),
            *GetName());
        return;
    }

    for (UStaticMeshComponent* Mesh : MeshComponents)
    {
        Mesh->SetPhysMaterialOverride(FloorMaterial);

        UE_LOG(LevelAreaGraphManagement, Log,
            TEXT("ALevelAreaActor >> Applied %s to %s on %s"),
            *FloorMaterial->GetName(), *Mesh->GetName(), *GetName());
    }
}


/* =====================================================================
   Lifecycle
   ===================================================================== */

void ALevelAreaActor::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
        RegisterWithSubsystem();
}

void ALevelAreaActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (HasAuthority())
        UnregisterFromSubsystem();

    Super::EndPlay(EndPlayReason);
}


/* =====================================================================
   Subsystem Registration
   ===================================================================== */

void ALevelAreaActor::RegisterWithSubsystem()
{
    if (NodeID == INDEX_NONE)
    {
        UE_LOG(LevelAreaGraphManagement, Error,
            TEXT("ALevelAreaActor >> %s has no NodeID set"), *GetName());
        return;
    }

    if (!FloorMaterial)
    {
        UE_LOG(LevelAreaGraphManagement, Warning,
            TEXT("ALevelAreaActor >> %s has no FloorMaterial — tracker cannot detect this node"),
            *GetName());
    }

    OwnedNode = MakeUnique<LevelAreaNode>();
    OwnedNode->SetNodeID(NodeID);
    OwnedNode->SetRoomCenter(GetActorLocation());

    for (int32 NeighborID : NeighborNodeIDs)
        OwnedNode->AddConnection(NeighborID,
            GetActorLocation(), GetActorLocation());

    ULevelAreaGraphSubsystem* Sub =
        GetWorld()->GetSubsystem<ULevelAreaGraphSubsystem>();
    if (!Sub) return;

    Sub->RegisterNode(OwnedNode.Get());

    if (FloorMaterial)
        Sub->RegisterFloorMaterial(FloorMaterial, NodeID);

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("ALevelAreaActor >> Node %d registered | Material: %s"),
        NodeID,
        FloorMaterial ? *FloorMaterial->GetName() : TEXT("none"));
}

void ALevelAreaActor::UnregisterFromSubsystem()
{
    if (!OwnedNode) return;

    ULevelAreaGraphSubsystem* Sub =
        GetWorld()->GetSubsystem<ULevelAreaGraphSubsystem>();
    if (!Sub) return;

    Sub->UnregisterNode(OwnedNode->GetNodeID());

    if (FloorMaterial)
        Sub->UnregisterFloorMaterial(FloorMaterial);

    OwnedNode.Reset();
}


/* =====================================================================
   Hazard State
   ===================================================================== */

void ALevelAreaActor::SetHazardState(EAreaHazardState NewState)
{
    if (!HasAuthority()) return;

    HazardState = NewState;
    OnHazardStateChanged.Broadcast(HazardState);

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("ALevelAreaActor >> Node %d state: %s"),
        NodeID, *UEnum::GetValueAsString(HazardState));
}

void ALevelAreaActor::OnRep_HazardState()
{
    OnHazardStateChanged.Broadcast(HazardState);

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("ALevelAreaActor >> Node %d replicated state: %s"),
        NodeID, *UEnum::GetValueAsString(HazardState));
}