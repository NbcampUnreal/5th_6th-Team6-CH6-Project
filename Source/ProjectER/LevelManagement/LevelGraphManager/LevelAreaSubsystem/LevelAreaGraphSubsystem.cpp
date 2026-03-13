#include "LevelAreaGraphSubsystem.h"
#include "LevelManagement/Requirements/LevelAreaGraphData.h"
#include "LogHelper/DebugLogHelper.h"


void ULevelAreaGraphSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("LevelAreaGraphSubsystem >> Initialize"));
}

void ULevelAreaGraphSubsystem::Deinitialize()
{
    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("LevelAreaGraphSubsystem >> Deinitialize"));

    for (TPair<int32, LevelAreaNode*>& Pair : Nodes)
    {
        delete Pair.Value;
    }

    Nodes.Reset();
    ActiveHazardNodes.Reset();

    Super::Deinitialize();
}

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
        TEXT("RegisterNode >> ID: %d | Total Nodes: %d"),
        NodeID, Nodes.Num());
}

void ULevelAreaGraphSubsystem::UnregisterNode(int32 NodeID)
{
    if (Nodes.Remove(NodeID) > 0)
    {
        ActiveHazardNodes.Remove(NodeID);

        UE_LOG(LevelAreaGraphManagement, Log,
            TEXT("UnregisterNode >> ID %d removed"), NodeID);
    }
}

LevelAreaNode* ULevelAreaGraphSubsystem::GetNode(int32 NodeID) const
{
    LevelAreaNode* const* Found = Nodes.Find(NodeID);

    if (!Found)
    {
        UE_LOG(LevelAreaGraphManagement, Warning,
            TEXT("GetNode >> NodeID %d not found"), NodeID);
        return nullptr;
    }

    return *Found;
}

TArray<LevelAreaNode*> ULevelAreaGraphSubsystem::GetAllAreaNodes() const
{
    TArray<LevelAreaNode*> Result;
    Result.Reserve(Nodes.Num());

    for (const TPair<int32, LevelAreaNode*>& Pair : Nodes)
    {
        Result.Add(Pair.Value);
    }

    return Result;
}

bool ULevelAreaGraphSubsystem::GenerateHazardOrder(
    int32 HazardCount,
    int32 Seed,
    TArray<int32>& OutHazardOrder)
{
    OutHazardOrder.Reset();

    if (Nodes.Num() == 0)
        return false;

    // Deterministic random generator
    FRandomStream RandomStream(Seed);

    // Pick random root (final surviving node)
    TArray<int32> NodeIDs;
    Nodes.GetKeys(NodeIDs);

    int32 RandomIndex = RandomStream.RandRange(0, NodeIDs.Num() - 1);
    LevelAreaNode* Root = Nodes[NodeIDs[RandomIndex]];

    if (!Root)
        return false;

    // BFS traversal
    TArray<int32> BFSOrder;
    TQueue<LevelAreaNode*> Queue;
    TSet<int32> Visited;

    Queue.Enqueue(Root);
    Visited.Add(Root->GetNodeID());

    while (!Queue.IsEmpty())
    {
        LevelAreaNode* Current;
        Queue.Dequeue(Current);

        int32 ID = Current->GetNodeID();
        BFSOrder.Add(ID);

        int32 NeighborCount = Current->GetNumNeighbors();

        // Collect neighbors first
        TArray<IGridNode*> Neighbors;

        for (int32 i = 0; i < NeighborCount; i++)
        {
            IGridNode* Neighbor =
                Current->GetNeighborPointerGraph(i, this);

            if (Neighbor)
                Neighbors.Add(Neighbor);
        }

        // Shuffle neighbors for random BFS traversal
        for (int32 i = Neighbors.Num() - 1; i > 0; i--)
        {
            int32 j = RandomStream.RandRange(0, i);
            Neighbors.Swap(i, j);
        }

        // Process neighbors
        for (IGridNode* NeighborNode : Neighbors)
        {
            int32 NeighborID = NeighborNode->GetNodeID();

            if (!Visited.Contains(NeighborID))
            {
                Visited.Add(NeighborID);

                LevelAreaNode* AreaNode =
                    static_cast<LevelAreaNode*>(NeighborNode);

                if (AreaNode)
                    Queue.Enqueue(AreaNode);
            }
        }
    }

    // Reverse BFS order to generate collapse order
    for (int32 i = BFSOrder.Num() - 1; i > 0; i--)
    {
        OutHazardOrder.Add(BFSOrder[i]);
    }

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("GenerateHazardOrder >> Generated full collapse order (%d nodes) | Seed: %d"),
        OutHazardOrder.Num(),
        Seed);

    // Log full collapse order
    FString OrderString;

    for (int32 i = 0; i < OutHazardOrder.Num(); i++)
    {
        OrderString += FString::Printf(TEXT("%d"), OutHazardOrder[i]);

        if (i < OutHazardOrder.Num() - 1)
            OrderString += TEXT(" -> ");
    }

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("Hazard Collapse Order: %s"),
        *OrderString);

    return true;
}

void ULevelAreaGraphSubsystem::ApplyHazardNodes(
    const TArray<int32>& NodeIDs,
    EAreaHazardState State)
{
    for (int32 ID : NodeIDs)
    {
        ActiveHazardNodes.Add(ID, State);

        if (LevelAreaNode* Node = GetNode(ID))
        {
            Node->SetFlag(1, true);
        }
    }

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("ApplyHazardNodes >> %d nodes set to %s"),
        NodeIDs.Num(),
        *UEnum::GetValueAsString(State));
}

void ULevelAreaGraphSubsystem::ClearHazards()
{
    for (const TPair<int32, EAreaHazardState>& Pair : ActiveHazardNodes)
    {
        if (LevelAreaNode* Node = GetNode(Pair.Key))
        {
            Node->SetFlag(1, false);
        }
    }

    ActiveHazardNodes.Reset();

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("ClearHazards >> All hazards cleared"));
}

bool ULevelAreaGraphSubsystem::GenerateHazardOrder_BP(int32 HazardCount, int32 Seed, TArray<int32>& OutHazardOrder,
    FString& ResultLog)
{
    ResultLog.Empty();
    OutHazardOrder.Reset();

    bool bResult = GenerateHazardOrder(HazardCount, Seed, OutHazardOrder);

    if (!bResult)
    {
        ResultLog = TEXT("GenerateHazardOrder failed.");
        return false;
    }

    ResultLog += FString::Printf(
        TEXT("Hazard Order Generated | Seed: %d\n"),
        Seed);

    ResultLog += TEXT("Collapse Order:\n");

    for (int32 i = 0; i < OutHazardOrder.Num(); i++)
    {
        ResultLog += FString::Printf(
            TEXT("Phase %d -> Node %d\n"),
            i + 1,
            OutHazardOrder[i]);
    }

    ResultLog += FString::Printf(
        TEXT("Final Safe Node Count: %d\n"),
        Nodes.Num() - OutHazardOrder.Num());

    return true;
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
   IGridGraph
   ===================================================================== */

TArray<IGridNode*> ULevelAreaGraphSubsystem::GetAllNodes()
{
    TArray<IGridNode*> Result;

    Result.Reserve(Nodes.Num());

    for (TPair<int32, LevelAreaNode*>& Pair : Nodes)
    {
        Result.Add(Pair.Value);
    }

    return Result;
}

IGridNode* ULevelAreaGraphSubsystem::FindNodeByID(int32 NodeID)
{
    return GetNode(NodeID);
}

IGridNode* ULevelAreaGraphSubsystem::FindNodeByLocation(const FVector& Location)
{
    float BestDistSq = FLT_MAX;
    LevelAreaNode* BestNode = nullptr;

    for (TPair<int32, LevelAreaNode*>& Pair : Nodes)
    {
        LevelAreaNode* Node = Pair.Value;

        if (!Node)
            continue;

        float DistSq = FVector::DistSquared(Node->GetWorldLocation(), Location);

        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            BestNode = Node;
        }
    }

    return BestNode;
}

void ULevelAreaGraphSubsystem::ResetAllNodes()
{
    for (TPair<int32, LevelAreaNode*>& Pair : Nodes)
    {
        Pair.Value->ResetState();
    }
}


/* =====================================================================
   Internal Graph Checks
   ===================================================================== */

bool ULevelAreaGraphSubsystem::WouldCreateIsland(int32 CandidateID)
{
    LevelAreaNode* Node = GetNode(CandidateID);

    if (!Node)
        return true;

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

    if (!StartNode)
        return true;

    TSet<int32> Visited;

    BFS(StartNode, Visited);

    int32 ValidCount = 0;

    for (const TPair<int32, LevelAreaNode*>& Pair : Nodes)
    {
        if (!Pair.Value->HasFlag(1))
        {
            ValidCount++;
        }
    }

    return Visited.Num() == ValidCount;
}

void ULevelAreaGraphSubsystem::BFS(
    LevelAreaNode* StartNode,
    TSet<int32>& Visited) const
{
    if (!StartNode)
        return;

    TQueue<LevelAreaNode*> Queue;

    Queue.Enqueue(StartNode);

    Visited.Add(StartNode->GetNodeID());

    while (!Queue.IsEmpty())
    {
        LevelAreaNode* Current = nullptr;

        Queue.Dequeue(Current);

        if (!Current)
            continue;

        for (int32 i = 0; i < Current->GetNumNeighbors(); i++)
        {
            IGridNode* NeighborBase =
                Current->GetNeighborPointerGraph(
                    i,
                    const_cast<ULevelAreaGraphSubsystem*>(this));

            LevelAreaNode* Neighbor =
                static_cast<LevelAreaNode*>(NeighborBase);

            if (!Neighbor || Neighbor->HasFlag(1))
                continue;

            int32 NeighborID = Neighbor->GetNodeID();

            if (!Visited.Contains(NeighborID))
            {
                Visited.Add(NeighborID);
                Queue.Enqueue(Neighbor);
            }
        }
    }
}