#include "Hex/HexAdapter.h"

// ---------------- FHexNodeAdapter ----------------

int32 FHexNodeAdapter::GetNumNeighbors() const
{
    if (!Owner) return 0;
    // Returns actual valid neighbors within grid bounds, not always 6
    return Owner->GetGridWrapper().Grid.GetNeighbors(Coord).Num();
}

IGridNode* FHexNodeAdapter::GetNeighborPointerGraph(int32 Index, IGridGraph* Graph) const
{
    // static_cast is safe — caller always passes UHexGridAdapter which implements IGridGraph
    UHexGridAdapter* HexGraph = static_cast<UHexGridAdapter*>(Graph);
    if (!HexGraph) return nullptr;

    const TArray<FHexCoord> Neighbors = HexGraph->GetGridWrapper().Grid.GetNeighbors(Coord);
    if (!Neighbors.IsValidIndex(Index)) return nullptr;

    return HexGraph->GetNode(Neighbors[Index]);
}

IGridNode* FHexNodeAdapter::GetNeighborIndexGraph(int32 Index, IGridGraph* Graph) const
{
    return GetNeighborPointerGraph(Index, Graph);
}

FVector FHexNodeAdapter::GetWorldLocation() const
{
    if (!Owner) return FVector::ZeroVector;
    return FVector(Owner->GetGridWrapper().GetWorldPos(Coord), 0.f);
}

float FHexNodeAdapter::GetCostTo(const IGridNode* TargetNode) const
{
    if (!TargetNode || !Owner) return 0.f;
    const FHexNodeAdapter* HexTarget = static_cast<const FHexNodeAdapter*>(TargetNode);
    return FVector2D::Distance(
        Owner->GetGridWrapper().GetWorldPos(Coord),
        Owner->GetGridWrapper().GetWorldPos(HexTarget->GetCoord()));
}

float FHexNodeAdapter::GetHeuristicCost(const IGridNode* TargetNode) const
{
    if (!TargetNode) return 0.f;
    const FHexNodeAdapter* HexTarget = static_cast<const FHexNodeAdapter*>(TargetNode);

    // Multiply by HexSize so heuristic is in world units — matches GetCostTo
    if (Owner)
        return static_cast<float>(Coord.DistanceTo(HexTarget->Coord)) * Owner->GetGridWrapper().Layout.HexSize;

    return static_cast<float>(Coord.DistanceTo(HexTarget->Coord));
}

bool FHexNodeAdapter::HasFlag(uint8 Flag) const
{
    return (Flags & Flag) != 0;
}

void FHexNodeAdapter::SetFlag(uint8 Flag, bool bValue)
{
    if (bValue) Flags |= Flag;
    else        Flags &= ~Flag;
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
    return static_cast<int32>(GetTypeHash(Coord));
}

// ---------------- UHexGridAdapter ----------------

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
    Result.Reserve(Nodes.Num());
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
    // WorldToHex is O(1) — avoids linear scan
    FVector2D Loc2D(Location.X, Location.Y);
    FHexCoord Coord = GridWrapper.Layout.WorldToHex(Loc2D);
    return GetNode(Coord);
}

void UHexGridAdapter::ResetAllNodes()
{
    for (auto& Pair : Nodes)
        Pair.Value->ResetState();
}

FHexNodeAdapter* UHexGridAdapter::GetNode(const FHexCoord& Coord) const
{
    const TUniquePtr<FHexNodeAdapter>* Found = Nodes.Find(Coord);
    return Found ? Found->Get() : nullptr;
}