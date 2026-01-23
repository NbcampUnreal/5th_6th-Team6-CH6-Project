// Fill out your copyright notice in the Description page of Project Settings.


#include "Hex/HexAdapter.h"

//============ FHexNodeAdapter =======================================================================================//

int32 FHexNodeAdapter::GetNumNeighbors() const
{
    if (!Owner) return 0;
    FHexNodeAdapter* Node = Owner->GetNode(Coord);
    return Node ? Owner->GetNode(Coord)->GetCoord().DistanceTo(Coord) : 0;
}

IGridNode* FHexNodeAdapter::GetNeighborPointerGraph(int32 Index, UObject* Graph) const
{
    UHexGridAdapter* HexGraph = Cast<UHexGridAdapter>(Graph);
    if (!HexGraph)
        return nullptr;

    // Then get the neighbor node from HexGraph
    const TArray<FHexCoord> Neighbors = HexGraph->GetGridWrapper().Grid.GetNeighbors(Coord);
    if (Index < 0 || Index >= Neighbors.Num())
        return nullptr;

    return HexGraph->GetNode(Neighbors[Index]);
}

IGridNode* FHexNodeAdapter::GetNeighborIndexGraph(int32 Index, UObject* Graph) const
{
    // Same as pointer-based lookup for this implementation
    return GetNeighborPointerGraph(Index, Graph);
}

FVector FHexNodeAdapter::GetWorldLocation() const
{
    if (!Owner) return FVector::ZeroVector;
    return FVector(Owner->GetGridWrapper().GetWorldPos(Coord), 0.f);
}

float FHexNodeAdapter::GetCostTo(const IGridNode* TargetNode) const
{
    if (!TargetNode) return 0.f;
    const FHexNodeAdapter* HexTarget = static_cast<const FHexNodeAdapter*>(TargetNode);
    return FVector2D::Distance(Owner->GetGridWrapper().GetWorldPos(Coord),
                               Owner->GetGridWrapper().GetWorldPos(HexTarget->GetCoord()));
}

float FHexNodeAdapter::GetHeuristicCost(const IGridNode* TargetNode) const
{
    const FHexNodeAdapter* HexTarget = static_cast<const FHexNodeAdapter*>(TargetNode);
    if (!HexTarget)
    {
        return 0.f;
    }

    return static_cast<float>(Coord.DistanceTo(HexTarget->Coord));
}

bool FHexNodeAdapter::HasFlag(uint8 Flag) const
{
    return (Flags & Flag) != 0;
}

void FHexNodeAdapter::SetFlag(uint8 Flag, bool bValue)
{
    if (bValue)
    {
        Flags |= Flag;
    }
    else
    {
        Flags &= ~Flag;
    }
}

void FHexNodeAdapter::ResetState()
{
    GCost  = 0.f;
    HCost  = 0.f;
    Flags  = 0;
    Parent = nullptr;
    Tag    = NAME_None;
}

int32 FHexNodeAdapter::GetNodeID() const
{
    // Stable hash-based ID
    return static_cast<int32>(GetTypeHash(Coord));
}

//============ FHexGridAdapter =======================================================================================//

void UHexGridAdapter::Initialize(const FHexGridWrapper& InGrid)
{
    GridWrapper = InGrid;
    BuildAdapters();
}

void UHexGridAdapter::BuildAdapters()
{
    Nodes.Empty();
    for (const FHexCoord& Hex : GridWrapper.Grid.Cells)
    {
        Nodes.Add(Hex, MakeUnique<FHexNodeAdapter>(Hex, this));
    }
}

TArray<IGridNode*> UHexGridAdapter::GetAllNodes()
{
    TArray<IGridNode*> Result;
    for (auto& Pair : Nodes)
        Result.Add(Pair.Value.Get());
    return Result;
}

IGridNode* UHexGridAdapter::FindNodeByID(int32 NodeID)
{
    for (auto& Pair : Nodes)
    {
        if (Pair.Value->GetNodeID() == NodeID)
            return Pair.Value.Get();
    }
    return nullptr;
}

IGridNode* UHexGridAdapter::FindNodeByLocation(const FVector& Location)
{
    FVector2D Loc2D(Location.X, Location.Y);
    
    for (auto& Pair : Nodes)
    {
        FVector NodeWorld = Pair.Value->GetWorldLocation(); // FVector
        FVector2D Node2D(NodeWorld.X, NodeWorld.Y);// convert to 2D
        if (FVector2D::Distance(Node2D, Loc2D) < KINDA_SMALL_NUMBER)
            return Pair.Value.Get();
    }
    return nullptr;
}

void UHexGridAdapter::ResetAllNodes()
{
    for (auto& Pair : Nodes)
        Pair.Value->ResetState();
}

FHexNodeAdapter* UHexGridAdapter::GetNode(const FHexCoord& Coord) const
{
    if (Nodes.Contains(Coord))
        return Nodes[Coord].Get();
    return nullptr;
}