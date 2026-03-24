#include "LevelAreaActor.h"

#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "LogHelper/DebugLogHelper.h"


#if WITH_EDITOR
#include "Editor.h" // for geditor
#endif

ALevelAreaActor::ALevelAreaActor()
{
    PrimaryActorTick.bCanEverTick = false;

    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}


/* =====================================================================
   Editor Utility
   ===================================================================== */

void ALevelAreaActor::ApplyMaterialToMeshes()
{
    if (!FloorMaterial)
    {
        UE_LOG(LevelAreaGraphManagement, Warning,
            TEXT("ALevelAreaActor >> ApplyMaterialToMeshes: FloorMaterial not set on %s"),
            *GetName());
        return;
    }

    TArray<AActor*> AttachedActors;
    GetAttachedActors(AttachedActors);

    if (AttachedActors.IsEmpty())
    {
        UE_LOG(LevelAreaGraphManagement, Warning,
            TEXT("ALevelAreaActor >> ApplyMaterialToMeshes: No attached actors found on %s"),
            *GetName());
        return;
    }

    for (AActor* Child : AttachedActors)
    {
        TArray<UStaticMeshComponent*> Meshes;
        Child->GetComponents<UStaticMeshComponent>(Meshes);

        for (UStaticMeshComponent* Mesh : Meshes)
        {
            //set physic material
            Mesh->SetPhysMaterialOverride(FloorMaterial);
            // set trace reaction for area ID tracker
            Mesh->SetCollisionResponseToChannel( FloorTraceChannel, ECR_Block);

            UE_LOG(LevelAreaGraphManagement, Log,
                TEXT("ALevelAreaActor >> Applied %s to %s on %s"),
                *FloorMaterial->GetName(), *Mesh->GetName(), *Child->GetName());
        }
    }
}

void ALevelAreaActor::BakeAllNodes()
{
#if WITH_EDITOR
    if (!GEditor) return;

    UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
    if (!EditorWorld) return;

    int32 BakedCount = 0;

    for (TActorIterator<ALevelAreaActor> It(EditorWorld); It; ++It)
    {
        ALevelAreaActor* Actor = *It;
        if (!Actor) continue;

        Actor->BakeNode();
        BakedCount++;
    }

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("ALevelAreaActor >> BakeAllNodes: Baked %d actors"), BakedCount);
#endif
}

void ALevelAreaActor::ClearGraphData()
{
#if WITH_EDITOR
    if (!GraphData)
    {
        UE_LOG(LevelAreaGraphManagement, Warning,
            TEXT("ALevelAreaActor >> ClearGraphData: No GraphData assigned on %s"),
            *GetName());
        return;
    }

    int32 RemovedCount = GraphData->Nodes.Num();
    GraphData->Nodes.Reset();
    
   bool DebugBoolChecker= GraphData->MarkPackageDirty();

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("ALevelAreaActor >> ClearGraphData: Cleared %d nodes from %s"),
        RemovedCount, *GraphData->GetName());
#endif
}


/* =====================================================================
   Internal
   ===================================================================== */

void ALevelAreaActor::BakeNode()
{
#if WITH_EDITOR
    if (NodeID == INDEX_NONE)
    {
        UE_LOG(LevelAreaGraphManagement, Error,
            TEXT("ALevelAreaActor >> BakeNode: NodeID not set on %s"), *GetName());
        return;
    }

    if (!GraphData)
    {
        UE_LOG(LevelAreaGraphManagement, Error,
            TEXT("ALevelAreaActor >> BakeNode: No GraphData assigned on %s"),
            *GetName());
        return;
    }

    FLevelAreaNodeRecord* Record = GraphData->Nodes.FindByPredicate(
        [this](const FLevelAreaNodeRecord& R) { return R.NodeID == NodeID; });

    if (!Record)
    {
        GraphData->Nodes.Add(FLevelAreaNodeRecord());
        Record = &GraphData->Nodes.Last();
        Record->NodeID = NodeID;
    }

    Record->NeighborIDs.Reset();

    for (ALevelAreaActor* Neighbor : NeighborActors)
    {
        if (!Neighbor || Neighbor->NodeID == INDEX_NONE)
        {
            UE_LOG(LevelAreaGraphManagement, Warning,
                TEXT("ALevelAreaActor >> BakeNode: Skipping invalid neighbor on %s"),
                *GetName());
            continue;
        }

        Record->NeighborIDs.AddUnique(Neighbor->NodeID);

        UE_LOG(LevelAreaGraphManagement, Log,
            TEXT("ALevelAreaActor >> BakeNode: Node %d → Neighbor %d"),
            NodeID, Neighbor->NodeID);
    }

    bool DebugBoolChecker = GraphData->MarkPackageDirty();

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("ALevelAreaActor >> BakeNode: Node %d baked | Neighbors: %d"),
        NodeID, Record->NeighborIDs.Num());
#endif
}