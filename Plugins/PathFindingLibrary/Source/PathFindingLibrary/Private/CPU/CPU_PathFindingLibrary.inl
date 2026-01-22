#pragma once

#include "CPU/CPU_PathFindingLibrary.h"
#include "Interface/GridNodeInterface.h"
#include "Containers/Queue.h"

template <typename NodeType>
float FCPU_PathFindingLibrary::GetDistance(NodeType* NodeA, NodeType* NodeB)
{
	if (!NodeA || !NodeB) return 0.f;
	// use interface function to get the result. so that it can be used universally
	return FVector::Dist(NodeA->GetWorldLocation(), NodeB->GetWorldLocation());
}

template <typename NodeType>
void FCPU_PathFindingLibrary::ResetNodeFlags(TArray<NodeType*>& Nodes, uint8 ResetMask)
{
	for (NodeType* Node : Nodes)
	{
		if (Node)
		{
			Node->SetFlag(ResetMask, false);
		}
	}
}


// Minimal local heap for pathfinding
template<typename T, typename FCompare>
class FMinHeap
{
    TArray<T> Data;
    FCompare Compare;

    void HeapifyUp(int32 Index)
    {
        while (Index > 0)
        {
            int32 Parent = (Index - 1) / 2;
            if (!Compare(Data[Index], Data[Parent])) break;
            Swap(Data[Index], Data[Parent]);
            Index = Parent;
        }
    }

    void HeapifyDown(int32 Index)
    {
        int32 Size = Data.Num();
        while (true)
        {
            int32 Left = Index * 2 + 1;
            int32 Right = Index * 2 + 2;
            int32 Best = Index;

            if (Left < Size && Compare(Data[Left], Data[Best])) Best = Left;
            if (Right < Size && Compare(Data[Right], Data[Best])) Best = Right;

            if (Best == Index) break;
            Swap(Data[Index], Data[Best]);
            Index = Best;
        }
    }

public:
    void Push(const T& Item) { Data.Add(Item); HeapifyUp(Data.Num() - 1); }
    T Pop() { T Top = Data[0]; Data[0] = Data.Last(); Data.Pop(); HeapifyDown(0); return Top; }
    bool IsEmpty() const { return Data.Num() == 0; }
};

// ---------------- A* Pathfinding ----------------

template<typename NodeType, typename GraphType>
bool FCPU_PathFindingLibrary::FindPath_AStar(
    GraphType* Graph,
    NodeType* StartNode,
    NodeType* GoalNode,
    TArray<NodeType*>& OutPath,
    TFunction<bool(NodeType*)> CanVisit)
{
   // Fuck. just make sure the graph is uobject. or else, it wont work
    static_assert(TIsDerivedFrom<GraphType, UObject>::Value, "GraphType must derive from UObject!");

    if (!Graph || !StartNode || !GoalNode) return false;

    OutPath.Reset();

    // --- Helper structs ---
    struct FNodeEntry 
    { 
        NodeType* Node; 
        float FCost; 
        FNodeEntry(NodeType* InNode, float InCost) : Node(InNode), FCost(InCost) {} 
    };

    struct FCompare 
    { 
        bool operator()(const FNodeEntry& A, const FNodeEntry& B) const 
        { 
            return A.FCost < B.FCost; 
        } 
    };

    FMinHeap<FNodeEntry, FCompare> OpenList;
    TMap<NodeType*, NodeType*> CameFrom;
    TMap<NodeType*, float> GScore;

    OpenList.Push(FNodeEntry(StartNode, 0.f));
    GScore.Add(StartNode, 0.f);

    while (!OpenList.IsEmpty())
    {
        FNodeEntry Current = OpenList.Pop();

        if (Current.Node == GoalNode)
        {
            // --- Reconstruct path ---
            NodeType* PathNode = GoalNode;
            while (PathNode)
            {
                OutPath.Insert(PathNode, 0);
                PathNode = CameFrom.Contains(PathNode) ? CameFrom[PathNode] : nullptr;
            }
            return true;
        }

        for (int32 i = 0; i < Current.Node->GetNumNeighbors(); ++i)
        {
            // --- Safe: GraphType is UObject, no Cast<> needed ---
            NodeType* Neighbor = Current.Node->GetNeighborPointerGraph(i, static_cast<UObject*>(Graph));

            if (!Neighbor || !Neighbor->IsTraversable()) continue;
            if (CanVisit && !CanVisit(Neighbor)) continue;

            float TentativeG = GScore[Current.Node] + Current.Node->GetCostTo(Neighbor);
            if (!GScore.Contains(Neighbor) || TentativeG < GScore[Neighbor])
            {
                GScore.Add(Neighbor, TentativeG);
                float FCost = TentativeG + Neighbor->GetHeuristicCost(GoalNode);
                OpenList.Push(FNodeEntry(Neighbor, FCost));
                CameFrom.Add(Neighbor, Current.Node);
            }
        }
    }

    return false;
}

// ---------------- Dijkstra Pathfinding ----------------

template<typename NodeType, typename GraphType>
bool FCPU_PathFindingLibrary::FindPath_Dijkstra(
    GraphType* Graph,
    NodeType* StartNode,
    NodeType* GoalNode,
    TArray<NodeType*>& OutPath,
    TFunction<bool(NodeType*)> CanVisit)
{
    if (!Graph || !StartNode || !GoalNode) return false;
    OutPath.Reset();

    struct FNodeEntry { NodeType* Node; float Cost; FNodeEntry(NodeType* InNode, float InCost) : Node(InNode), Cost(InCost) {} };
    struct FCompare { bool operator()(const FNodeEntry& A, const FNodeEntry& B) const { return A.Cost < B.Cost; } };

    FMinHeap<FNodeEntry, FCompare> OpenList;
    TMap<NodeType*, NodeType*> CameFrom;
    TMap<NodeType*, float> CostSoFar;

    OpenList.Push(FNodeEntry(StartNode, 0.f));
    CostSoFar.Add(StartNode, 0.f);

    while (!OpenList.IsEmpty())
    {
        FNodeEntry Current = OpenList.Pop();

        if (Current.Node == GoalNode)
        {
            NodeType* PathNode = GoalNode;
            while (PathNode)
            {
                OutPath.Insert(PathNode, 0);
                PathNode = CameFrom.Contains(PathNode) ? CameFrom[PathNode] : nullptr;
            }
            return true;
        }

        for (int32 i = 0; i < Current.Node->GetNumNeighbors(); ++i)
        {
            NodeType* Neighbor = Current.Node->GetNeighborPointerGraph(i, Graph);
            if (!Neighbor || !Neighbor->IsTraversable()) continue;
            if (CanVisit && !CanVisit(Neighbor)) continue;

            float NewCost = CostSoFar[Current.Node] + Current.Node->GetCostTo(Neighbor);
            if (!CostSoFar.Contains(Neighbor) || NewCost < CostSoFar[Neighbor])
            {
                CostSoFar.Add(Neighbor, NewCost);
                OpenList.Push(FNodeEntry(Neighbor, NewCost));
                CameFrom.Add(Neighbor, Current.Node);
            }
        }
    }

    return false;
}

// ---------------- BFS ----------------

template<typename NodeType, typename GraphType>
bool FCPU_PathFindingLibrary::BFS(
    GraphType* Graph,
    NodeType* StartNode,
    TArray<NodeType*>& OutConnectedNodes,
    TFunction<bool(NodeType*)> CanVisit)
{
    if (!Graph || !StartNode) return false;
    OutConnectedNodes.Reset();

    TQueue<NodeType*> Queue;
    TSet<NodeType*> Visited;

    Queue.Enqueue(StartNode);
    Visited.Add(StartNode);

    while (!Queue.IsEmpty())
    {
        NodeType* Current;
        Queue.Dequeue(Current);
        if (!Current) continue;

        OutConnectedNodes.Add(Current);

        for (int32 i = 0; i < Current->GetNumNeighbors(); ++i)
        {
            NodeType* Neighbor = Current->GetNeighborPointerGraph(i, Graph);
            if (!Neighbor || !Neighbor->IsTraversable() || Visited.Contains(Neighbor)) continue;
            if (CanVisit && !CanVisit(Neighbor)) continue;

            Queue.Enqueue(Neighbor);
            Visited.Add(Neighbor);
        }
    }

    return OutConnectedNodes.Num() > 0;
}