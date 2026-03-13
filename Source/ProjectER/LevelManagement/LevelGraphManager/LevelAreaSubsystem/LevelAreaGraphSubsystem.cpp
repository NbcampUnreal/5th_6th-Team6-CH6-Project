#include "LevelAreaGraphSubsystem.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Components/StaticMeshComponent.h"
#include "LogHelper/DebugLogHelper.h"

/* =====================================================================
   Graph Building
   ===================================================================== */

void ULevelAreaGraphSubsystem::RegisterNode(LevelAreaNode* Node)
{
    if (!Node)
    {
        UE_LOG(LevelAreaGraphManagement, Error,
            TEXT("RegisterNode >> Node is nullptr"));
        return;
    }

    int32 NodeID = Node->GetNodeID();
    Nodes.Add(NodeID, Node);

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("RegisterNode >> ID: %d | Total: %d"), NodeID, Nodes.Num());
}

void ULevelAreaGraphSubsystem::UnregisterNode(int32 NodeID)
{
    if (Nodes.Remove(NodeID) > 0)
    {
        ActiveHazardNodes.Remove(NodeID);

        UE_LOG(LevelAreaGraphManagement, Log,
            TEXT("UnregisterNode >> ID: %d removed"), NodeID);
    }
}

LevelAreaNode* ULevelAreaGraphSubsystem::GetNode(int32 NodeID) const
{
    const LevelAreaNode* const* Found = Nodes.Find(NodeID);

    if (!Found)
    {
        UE_LOG(LevelAreaGraphManagement, Warning,
            TEXT("GetNode >> NodeID %d not found"), NodeID);
        return nullptr;
    }

    return const_cast<LevelAreaNode*>(*Found);
}

TArray<LevelAreaNode*> ULevelAreaGraphSubsystem::GetAllAreaNodes() const
{
    TArray<LevelAreaNode*> Result;
    Result.Reserve(Nodes.Num());

    for (const TPair<int32, LevelAreaNode*>& Pair : Nodes)
        Result.Add(Pair.Value);

    return Result;
}


/* =====================================================================
   Hazard System
   ===================================================================== */

bool ULevelAreaGraphSubsystem::GenerateHazardOrder(
    int32 HazardCount,
    TArray<int32>& OutHazardOrder)
{
    OutHazardOrder.Reset();

    if (Nodes.Num() == 0)
    {
        UE_LOG(LevelAreaGraphManagement, Warning,
            TEXT("GenerateHazardOrder >> No nodes registered"));
        return false;
    }

    TArray<int32> Candidates;
    for (const TPair<int32, LevelAreaNode*>& Pair : Nodes)
        Candidates.Add(Pair.Key);

    Candidates.Sort([](int32, int32) { return FMath::RandBool(); });

    for (int32 NodeID : Candidates)
    {
        if (OutHazardOrder.Num() >= HazardCount) break;

        if (!WouldCreateIsland(NodeID))
        {
            OutHazardOrder.Add(NodeID);

            if (LevelAreaNode* Node = GetNode(NodeID))
                Node->SetFlag(1, true);
        }
    }

    // Clean up temporary flags
    for (const TPair<int32, LevelAreaNode*>& Pair : Nodes)
        Pair.Value->SetFlag(1, false);

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("GenerateHazardOrder >> Generated %d / %d hazards"),
        OutHazardOrder.Num(), HazardCount);

    return OutHazardOrder.Num() == HazardCount;
}

void ULevelAreaGraphSubsystem::ApplyHazardNodes(
    const TArray<int32>& NodeIDs, EAreaHazardState State)
{
    for (int32 ID : NodeIDs)
    {
        ActiveHazardNodes.Add(ID, State);

        if (LevelAreaNode* Node = GetNode(ID))
            Node->SetFlag(1, true);
    }

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("ApplyHazardNodes >> %d nodes set to %s"),
        NodeIDs.Num(), *UEnum::GetValueAsString(State));
}

void ULevelAreaGraphSubsystem::ClearHazards()
{
    for (const TPair<int32, EAreaHazardState>& Pair : ActiveHazardNodes)
    {
        if (LevelAreaNode* Node = GetNode(Pair.Key))
            Node->SetFlag(1, false);
    }

    ActiveHazardNodes.Reset();

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("ClearHazards >> All hazards cleared"));
}


/* =====================================================================
   Player Query
   ===================================================================== */

EAreaHazardState ULevelAreaGraphSubsystem::GetNodeHazardState(int32 NodeID) const
{
    const EAreaHazardState* Found = ActiveHazardNodes.Find(NodeID);
    return Found ? *Found : EAreaHazardState::Safe;
}

bool ULevelAreaGraphSubsystem::IsNodeHazard(int32 NodeID) const
{
    return ActiveHazardNodes.Contains(NodeID);
}


/* =====================================================================
   Surface → Node Mapping
   ===================================================================== */

void ULevelAreaGraphSubsystem::RegisterFloorMaterial(
    UPhysicalMaterial* Material, int32 NodeID)
{
    if (!Material) return;

    MaterialToNodeID.Add(Material, NodeID);

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("RegisterFloorMaterial >> %s → Node %d"),
        *Material->GetName(), NodeID);
}

void ULevelAreaGraphSubsystem::RegisterFloorMesh(
    UStaticMesh* Mesh, int32 NodeID)
{
    if (!Mesh) return;

    MeshToNodeID.Add(Mesh, NodeID);

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("RegisterFloorMesh >> %s → Node %d"),
        *Mesh->GetName(), NodeID);
}

void ULevelAreaGraphSubsystem::UnregisterFloorMaterial(UPhysicalMaterial* Material)
{
    if (!Material) return;
    MaterialToNodeID.Remove(Material);
}

void ULevelAreaGraphSubsystem::UnregisterFloorMesh(UStaticMesh* Mesh)
{
    if (!Mesh) return;
    MeshToNodeID.Remove(Mesh);
}

int32 ULevelAreaGraphSubsystem::FindNodeByHitResult(const FHitResult& Hit) const
{
    // Material takes priority — more specific than mesh
    if (UPhysicalMaterial* PhysMat = Hit.PhysMaterial.Get())
    {
        if (const int32* Found = MaterialToNodeID.Find(PhysMat))
            return *Found;
    }

    // Fall back to mesh identity
    if (UStaticMeshComponent* SMC =
        Cast<UStaticMeshComponent>(Hit.GetComponent()))
    {
        if (const int32* Found = MeshToNodeID.Find(SMC->GetStaticMesh()))
            return *Found;
    }

    UE_LOG(LevelAreaGraphManagement, Verbose,
        TEXT("FindNodeByHitResult >> No node matched for hit on %s"),
        Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("null"));

    return INDEX_NONE;
}


/* =====================================================================
   IGridGraph
   ===================================================================== */

TArray<IGridNode*> ULevelAreaGraphSubsystem::GetAllNodes()
{
    TArray<IGridNode*> Result;
    Result.Reserve(Nodes.Num());

    for (TPair<int32, LevelAreaNode*>& Pair : Nodes)
        Result.Add(Pair.Value);

    return Result;
}

IGridNode* ULevelAreaGraphSubsystem::FindNodeByID(int32 NodeID)
{
    return GetNode(NodeID);
}

IGridNode* ULevelAreaGraphSubsystem::FindNodeByLocation(const FVector& Location)
{
    LevelAreaNode* Closest = nullptr;
    float BestDist = MAX_FLT;

    for (const TPair<int32, LevelAreaNode*>& Pair : Nodes)
    {
        float Dist = FVector::DistSquared(Pair.Value->GetWorldLocation(), Location);
        if (Dist < BestDist)
        {
            BestDist = Dist;
            Closest = Pair.Value;
        }
    }

    return Closest;
}

void ULevelAreaGraphSubsystem::ResetAllNodes()
{
    for (TPair<int32, LevelAreaNode*>& Pair : Nodes)
        Pair.Value->ResetState();
}


/* =====================================================================
   Internal Graph Checks
   ===================================================================== */

bool ULevelAreaGraphSubsystem::WouldCreateIsland(int32 CandidateID)
{
    LevelAreaNode* Node = GetNode(CandidateID);
    if (!Node) return true;

    Node->SetFlag(1, true);
    bool bConnected = IsGraphConnected();
    Node->SetFlag(1, false);

    return !bConnected;
}

bool ULevelAreaGraphSubsystem::IsGraphConnected() const
{
    LevelAreaNode* StartNode = nullptr;

    for (const TPair<int32, LevelAreaNode*>& Pair : Nodes)
    {
        if (!Pair.Value->HasFlag(1))
        {
            StartNode = Pair.Value;
            break;
        }
    }

    if (!StartNode) return true;

    TSet<int32> Visited;
    BFS(StartNode, Visited);

    int32 ValidCount = 0;
    for (const TPair<int32, LevelAreaNode*>& Pair : Nodes)
    {
        if (!Pair.Value->HasFlag(1)) ValidCount++;
    }

    return Visited.Num() == ValidCount;
}

void ULevelAreaGraphSubsystem::BFS(LevelAreaNode* StartNode, TSet<int32>& Visited) const
{
    if (!StartNode) return;

    TQueue<LevelAreaNode*> Queue;
    Queue.Enqueue(StartNode);
    Visited.Add(StartNode->GetNodeID());

    while (!Queue.IsEmpty())
    {
        LevelAreaNode* Current = nullptr;
        Queue.Dequeue(Current);
        if (!Current) continue;

        for (int32 i = 0; i < Current->GetNumNeighbors(); i++)
        {
            IGridNode* NeighborBase = Current->GetNeighborPointerGraph(
                i, const_cast<ULevelAreaGraphSubsystem*>(this));

            LevelAreaNode* Neighbor = static_cast<LevelAreaNode*>(NeighborBase);
            if (!Neighbor || Neighbor->HasFlag(1)) continue;

            int32 NeighborID = Neighbor->GetNodeID();
            if (!Visited.Contains(NeighborID))
            {
                Visited.Add(NeighborID);
                Queue.Enqueue(Neighbor);
            }
        }
    }
}