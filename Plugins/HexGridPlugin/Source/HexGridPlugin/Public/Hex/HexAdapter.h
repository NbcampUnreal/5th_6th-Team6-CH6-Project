// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CPU/Interface/GridNodeInterface.h"
#include "Templates/UniquePtr.h"
#include "HexagonData.h"// making a wrapper data to use it for hexagon path finding
#include "HexAdapter.generated.h"

/**
 *  Think of this as a translator for struct data to interface. this keeps the data as it is, and allows to use interface
 */


// Forward
class UHexGridAdapter;

class FHexNodeAdapter : public IGridNode
{
public:
    FHexNodeAdapter(const FHexCoord& InCoord, UHexGridAdapter* InOwner)
        : Coord(InCoord)
        , Owner(InOwner)
    {}

    // ---------------- Core ----------------
    virtual int32 GetNumNeighbors() const override;
    virtual IGridNode* GetNeighborPointerGraph(int32 Index, UObject* Graph) const override;
    virtual IGridNode* GetNeighborIndexGraph(int32 Index, UObject* Graph) const override;

    virtual FVector GetWorldLocation() const override;

    // ---------------- Cost ----------------
    virtual float GetCostTo(const IGridNode* TargetNode) const override;
    virtual void SetCost(float Cost) override { GCost = Cost; }

    virtual float GetHeuristicCost(const IGridNode* TargetNode) const override;
    virtual void SetHeuristicCost(float Cost) override { HCost = Cost; }

    // ---------------- Flags ----------------
    virtual bool HasFlag(uint8 Flag) const override;
    virtual void SetFlag(uint8 Flag, bool bValue) override;
    virtual bool IsTraversable() const override { return !HasFlag(1); }

    // ---------------- Path ----------------
    virtual IGridNode* GetParent() const override { return Parent; }
    virtual void SetParent(IGridNode* InParent) override { Parent = InParent; }

    // ---------------- Utility ----------------
    virtual void ResetState() override;
    virtual int32 GetNodeID() const override;
    virtual void SetTag(FName InTag) override { Tag = InTag; }
    virtual FName GetTag() const override { return Tag; }

    const FHexCoord& GetCoord() const { return Coord; }

private:
    const FHexCoord& Coord;// only reference. not a copy!!
    UHexGridAdapter* Owner;

    // Pathfinding state (NOT in HexCoord)
    float GCost = 0.f;
    float HCost = 0.f;
    uint8 Flags = 0;
    IGridNode* Parent = nullptr;
    FName Tag;
};


//Same for the grid graph

UCLASS(BlueprintType)
class HEXGRIDPLUGIN_API UHexGridAdapter : public UObject, public IGridGraph
{
    GENERATED_BODY()
public:
    UHexGridAdapter() {}

    void Initialize(const FHexGridWrapper& InGrid);

    // ---------- IGridGraph ----------
    virtual TArray<IGridNode*> GetAllNodes() override;
    virtual IGridNode* FindNodeByID(int32 NodeID) override;
    virtual IGridNode* FindNodeByLocation(const FVector& Location) override;
    virtual void ResetAllNodes() override;

    FHexNodeAdapter* GetNode(const FHexCoord& Coord) const;
    FHexGridWrapper GetGridWrapper()const {return GridWrapper;}
    
private:
    FHexGridWrapper GridWrapper;
    
    TMap<FHexCoord, TUniquePtr<FHexNodeAdapter>> Nodes;

    void BuildAdapters();
};



/**
 * Per-query / per-agent pathfinding context.
 * this is for runing the path finding without rebuilding the grid every time when the function is used
 *
 *  simply saying -> It is for caching a pathfinding view of the grid.
 *  fuck yeah
 */

struct FHexPathQueryContext
{
    UHexGridAdapter* Graph;

    FHexPathQueryContext(const FHexGridWrapper& Grid, UObject* Outer)
    {
        // Create the UObject properly
        Graph = NewObject<UHexGridAdapter>(Outer);
        Graph->Initialize(Grid);
    }

    void Reset()
    {
        if (Graph)
        {
            Graph->ResetAllNodes();
        }
    }
};