// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelAreaGraphSubsystem.h"


void ULevelAreaGraphSubsystem::RegisterNode(LevelAreaNode* Node)
{
    if (!Node)
        return;

    Nodes.Add(Node->GetNodeID(), Node);
}

LevelAreaNode* ULevelAreaGraphSubsystem::GetNode(int32 NodeID) const
{
    const LevelAreaNode* const* Found = Nodes.Find(NodeID);

    return Found ? const_cast<LevelAreaNode*>(*Found) : nullptr;
}

TArray<LevelAreaNode*> ULevelAreaGraphSubsystem::GetAllNodes() const
{
    TArray<LevelAreaNode*> Result;

    for (const TPair<int32, LevelAreaNode*>& Pair : Nodes)
    {
        Result.Add(Pair.Value);
    }

    return Result;
}

bool ULevelAreaGraphSubsystem::GenerateHazardOrder(
    int32 HazardCount,
    TArray<int32>& OutHazardOrder)
{
    OutHazardOrder.Reset();

    if (Nodes.Num() == 0)
        return false;

    // gather candidate IDs
    TArray<int32> Candidates;

    for (const TPair<int32, LevelAreaNode*>& Pair : Nodes)
    {
        Candidates.Add(Pair.Key);
    }

    // randomize candidate order
    Candidates.Sort([](int32 A, int32 B)
    {
        return FMath::RandBool();
    });

    for (int32 NodeID : Candidates)
    {
        if (OutHazardOrder.Num() >= HazardCount)
            break;

        if (!WouldCreateIsland(NodeID))
        {
            OutHazardOrder.Add(NodeID);

            LevelAreaNode* Node = GetNode(NodeID);
            if (Node)
            {
                // mark hazard temporarily
                Node->SetFlag(1, true);
            }
        }
    }

    // reset hazard flags after generation
    for (const TPair<int32, LevelAreaNode*>& Pair : Nodes)
    {
        Pair.Value->SetFlag(1, false);
    }

    return OutHazardOrder.Num() == HazardCount;
}

bool ULevelAreaGraphSubsystem::WouldCreateIsland(int32 CandidateID)
{
    LevelAreaNode* Node = GetNode(CandidateID);

    if (!Node)
        return true;

    // temporarily mark as hazard
    Node->SetFlag(1, true);

    bool bConnected = IsGraphConnected();

    // restore state
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

    int32 ValidNodeCount = 0;

    for (const TPair<int32, LevelAreaNode*>& Pair : Nodes)
    {
        if (!Pair.Value->HasFlag(1))
            ValidNodeCount++;
    }

    return Visited.Num() == ValidNodeCount;
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

        int32 NeighborCount = Current->GetNumNeighbors();

        for (int32 i = 0; i < NeighborCount; i++)
        {
            IGridNode* NeighborBase =
                Current->GetNeighborPointerGraph(i, nullptr);

            LevelAreaNode* Neighbor =
                static_cast<LevelAreaNode*>(NeighborBase);

            if (!Neighbor)
                continue;

            if (Neighbor->HasFlag(1))
                continue;

            if (!Visited.Contains(Neighbor->GetNodeID()))
            {
                Visited.Add(Neighbor->GetNodeID());
                Queue.Enqueue(Neighbor);
            }
        }
    }
}