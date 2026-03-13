#pragma once

#include "CoreMinimal.h"
#include "CPU/Interface/GridNodeInterface.h"

class PROJECTER_API LevelAreaNode : public IGridNode
{
private:

    int32         NodeID     = INDEX_NONE;
    TArray<int32> NeighborIDs;

    // Interface stubs — required by IGridNode but unused in this system
    float      Cost       = 0.f;
    float      Heuristic  = 0.f;
    uint8      Flags      = 0;
    IGridNode* ParentNode = nullptr;
    FName      Tag;


public:

    /* ================================
       Setup
    ================================= */

    void SetNodeID(int32 ID)     { NodeID = ID; }

    void AddNeighbor(int32 TargetID)    { NeighborIDs.AddUnique(TargetID); }
    void RemoveNeighbor(int32 TargetID) { NeighborIDs.Remove(TargetID);    }

    const TArray<int32>& GetNeighborIDs() const { return NeighborIDs; }


    /* ================================
       IGridNode
    ================================= */

    virtual int32 GetNodeID() const override { return NodeID; }

    virtual int32 GetNumNeighbors() const override { return NeighborIDs.Num(); }

    virtual IGridNode* GetNeighborPointerGraph(
        int32 Index, IGridGraph* Graph) const override;

    virtual IGridNode* GetNeighborIndexGraph(
        int32 Index, IGridGraph* Graph) const override;

    // Unused — node has no spatial data
    virtual FVector GetWorldLocation() const override { return FVector::ZeroVector; }

    // Unused — no cost-based pathfinding
    virtual float GetCostTo(const IGridNode*) const override       { return 0.f; }
    virtual void  SetCost(float InCost) override                   { Cost = InCost; }
    virtual float GetHeuristicCost(const IGridNode*) const override { return 0.f; }
    virtual void  SetHeuristicCost(float InCost) override          { Heuristic = InCost; }

    virtual bool HasFlag(uint8 Flag) const override { return (Flags & Flag) != 0; }
    virtual void SetFlag(uint8 Flag, bool bValue) override
    {
        if (bValue) Flags |=  Flag;
        else        Flags &= ~Flag;
    }

    virtual bool IsTraversable() const override { return !HasFlag(1 << 0); }

    virtual IGridNode* GetParent() const override      { return ParentNode; }
    virtual void SetParent(IGridNode* Parent) override { ParentNode = Parent; }

    virtual void ResetState() override
    {
        Cost = 0; Heuristic = 0; ParentNode = nullptr; Flags = 0;
    }

    virtual void  SetTag(FName InTag) override { Tag = InTag; }
    virtual FName GetTag() const override      { return Tag;  }
};