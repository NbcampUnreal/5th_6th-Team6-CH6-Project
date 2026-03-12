#include "LevelAreaGraphSubsystem.h"
#include "LogHelper/DebugLogHelper.h"

void ULevelAreaGraphSubsystem::RegisterNode(LevelAreaNode* Node)
{
    if (!Node)
    {
        UE_LOG(LevelAreaGraphManagement, Error,
            TEXT("ULevelAreaGraphSubsystem::RegisterNode >> Node is nullptr"));
        return;
    }

    int32 NodeID = Node->GetNodeID();

    Nodes.Add(NodeID, Node);

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("ULevelAreaGraphSubsystem::RegisterNode >> Node Registered | ID: %d | Total Nodes: %d"),
        NodeID,
        Nodes.Num());
}

LevelAreaNode* ULevelAreaGraphSubsystem::GetNode(int32 NodeID) const
{
    const LevelAreaNode* const* Found = Nodes.Find(NodeID);

    if (!Found)
    {
        UE_LOG(LevelAreaGraphManagement, Warning,
            TEXT("ULevelAreaGraphSubsystem::GetNode >> GetNode failed | NodeID %d not found"),
            NodeID);

        return nullptr;
    }

    return const_cast<LevelAreaNode*>(*Found);
}

TArray<LevelAreaNode*> ULevelAreaGraphSubsystem::GetAllNodes() const
{
    TArray<LevelAreaNode*> Result;

    for (const TPair<int32, LevelAreaNode*>& Pair : Nodes)
    {
        Result.Add(Pair.Value);
    }

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("ULevelAreaGraphSubsystem::GetNode >> NodeCount: %d"),
        Result.Num());

    return Result;
}

bool ULevelAreaGraphSubsystem::GenerateHazardOrder(
    int32 HazardCount,
    TArray<int32>& OutHazardOrder)
{
    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("ULevelAreaGraphSubsystem::GenerateHazardOrder >> Start | Requested Count: %d"),
        HazardCount);

    OutHazardOrder.Reset();

    if (Nodes.Num() == 0)
    {
        UE_LOG(LevelAreaGraphManagement, Warning,
            TEXT("ULevelAreaGraphSubsystem::GenerateHazardOrder >> No nodes registered"));

        return false;
    }

    TArray<int32> Candidates;

    for (const TPair<int32, LevelAreaNode*>& Pair : Nodes)
    {
        Candidates.Add(Pair.Key);
    }

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("ULevelAreaGraphSubsystem::GenerateHazardOrder >> Candidate Node Count: %d"),
        Candidates.Num());

    // Randomize
    Candidates.Sort([](int32 A, int32 B)
    {
        return FMath::RandBool();
    });

    for (int32 NodeID : Candidates)
    {
        if (OutHazardOrder.Num() >= HazardCount)
            break;

        UE_LOG(LevelAreaGraphManagement, Verbose,
            TEXT(" ULevelAreaGraphSubsystem::GenerateHazardOrder >> Testing Node %d for hazard"),
            NodeID);

        if (!WouldCreateIsland(NodeID))
        {
            OutHazardOrder.Add(NodeID);

            UE_LOG(LevelAreaGraphManagement, Log,
                TEXT("ULevelAreaGraphSubsystem::GenerateHazardOrder >> Node %d accepted as hazard"),
                NodeID);

            LevelAreaNode* Node = GetNode(NodeID);

            if (Node)
            {
                Node->SetFlag(1, true);
            }
        }
        else
        {
            UE_LOG(LevelAreaGraphManagement, Verbose,
                TEXT("ULevelAreaGraphSubsystem::GenerateHazardOrder >> Node %d rejected (would create island)"),
                NodeID);
        }
    }

    for (const TPair<int32, LevelAreaNode*>& Pair : Nodes)
    {
        Pair.Value->SetFlag(1, false);
    }

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("ULevelAreaGraphSubsystem::GenerateHazardOrder >> Hazard Order Generated | Count: %d"),
        OutHazardOrder.Num());

    for (int32 ID : OutHazardOrder)
    {
        UE_LOG(LevelAreaGraphManagement, Log,
            TEXT("ULevelAreaGraphSubsystem::GenerateHazardOrder >> Hazard Node: %d"),
            ID);
    }

    return OutHazardOrder.Num() == HazardCount;
}

bool ULevelAreaGraphSubsystem::WouldCreateIsland(int32 CandidateID)
{
    LevelAreaNode* Node = GetNode(CandidateID);

    if (!Node)
    {
        UE_LOG(LevelAreaGraphManagement, Warning,
            TEXT("ULevelAreaGraphSubsystem::WouldCreateIsland >> Node %d not found"),
            CandidateID);

        return true;
    }

    UE_LOG(LevelAreaGraphManagement, Verbose,
        TEXT("ULevelAreaGraphSubsystem::WouldCreateIsland >> Testing island creation for Node %d"),
        CandidateID);

    Node->SetFlag(1, true);

    bool bConnected = IsGraphConnected();

    Node->SetFlag(1, false);

    UE_LOG(LevelAreaGraphManagement, Verbose,
        TEXT("ULevelAreaGraphSubsystem::WouldCreateIsland >> Island Test Node %d | Graph Connected: %s"),
        CandidateID,
        bConnected ? TEXT("YES") : TEXT("NO"));

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
    {
        UE_LOG(LevelAreaGraphManagement, Warning,
            TEXT("ULevelAreaGraphSubsystem::IsGraphConnected >> No valid start node"));

        return true;
    }

    UE_LOG(LevelAreaGraphManagement, Verbose,
        TEXT("ULevelAreaGraphSubsystem::IsGraphConnected >> Connectivity Test Starting from Node %d"),
        StartNode->GetNodeID());

    TSet<int32> Visited;

    BFS(StartNode, Visited);

    int32 ValidNodeCount = 0;

    for (const TPair<int32, LevelAreaNode*>& Pair : Nodes)
    {
        if (!Pair.Value->HasFlag(1))
            ValidNodeCount++;
    }

    bool bConnected = Visited.Num() == ValidNodeCount;

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("ULevelAreaGraphSubsystem::IsGraphConnected >> Connectivity Result | Visited: %d | Valid: %d | Connected: %s"),
        Visited.Num(),
        ValidNodeCount,
        bConnected ? TEXT("YES") : TEXT("NO"));

    return bConnected;
}

void ULevelAreaGraphSubsystem::BFS(
    LevelAreaNode* StartNode,
    TSet<int32>& Visited) const
{
    if (!StartNode)
        return;

    UE_LOG(LevelAreaGraphManagement, Verbose,
        TEXT("ULevelAreaGraphSubsystem::BFS >> BFS Start Node: %d"),
        StartNode->GetNodeID());

    TQueue<LevelAreaNode*> Queue;

    Queue.Enqueue(StartNode);
    Visited.Add(StartNode->GetNodeID());

    while (!Queue.IsEmpty())
    {
        LevelAreaNode* Current = nullptr;
        Queue.Dequeue(Current);

        if (!Current)
            continue;

        int32 CurrentID = Current->GetNodeID();

        UE_LOG(LevelAreaGraphManagement, Verbose,
            TEXT("ULevelAreaGraphSubsystem::BFS >> BFS Visiting Node %d"),
            CurrentID);

        int32 NeighborCount = Current->GetNumNeighbors();

        for (int32 i = 0; i < NeighborCount; i++)
        {
            IGridNode* NeighborBase =
                Current->GetNeighborPointerGraph(i, nullptr);

            LevelAreaNode* Neighbor =
                static_cast<LevelAreaNode*>(NeighborBase);

            if (!Neighbor)
                continue;

            int32 NeighborID = Neighbor->GetNodeID();

            if (Neighbor->HasFlag(1))
            {
                UE_LOG(LevelAreaGraphManagement, Verbose,
                    TEXT("ULevelAreaGraphSubsystem::BFS >> Neighbor %d skipped (hazard flag)"),
                    NeighborID);

                continue;
            }

            if (!Visited.Contains(NeighborID))
            {
                UE_LOG(LevelAreaGraphManagement, Verbose,
                    TEXT("ULevelAreaGraphSubsystem::BFS >> Neighbor %d queued"),
                    NeighborID);

                Visited.Add(NeighborID);
                Queue.Enqueue(Neighbor);
            }
        }
    }
}