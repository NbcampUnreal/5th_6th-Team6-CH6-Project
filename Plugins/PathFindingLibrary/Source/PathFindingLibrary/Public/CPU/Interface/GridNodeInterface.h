// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
//#include "UObject/Interface.h"
//#include "GridNodeInterface.generated.h"
//!!!!! this UInterface only works with class, so, to use it with struct, make this c++ code

/**
 * Required functions for making generic pathfinding
 *
 * Decouple the data type and details of the grid type
 * and share the logic of the pathfinding.
 */


//============= Node ============//
class IGridNode
{
public:
    virtual ~IGridNode() {}

    // ---------------- Core Node Functions ----------------
    virtual int32 GetNumNeighbors() const = 0;
    virtual IGridNode* GetNeighborPointerGraph(int32 Index, UObject* Graph) const = 0;
    virtual IGridNode* GetNeighborIndexGraph(int32 Index, UObject* Graph) const = 0;

    virtual FVector GetWorldLocation() const = 0;

    // ---------------- Cost / Pathfinding ----------------
    virtual float GetCostTo(const IGridNode* TargetNode) const = 0; // traversal cost
    virtual void SetCost(float Cost) = 0;

    virtual float GetHeuristicCost(const IGridNode* TargetNode) const = 0;
    virtual void SetHeuristicCost(float Cost) = 0;

    // ---------------- Flags / Traversal ----------------
    virtual bool HasFlag(uint8 Flag) const = 0;
    virtual void SetFlag(uint8 Flag, bool bValue) = 0;
    virtual bool IsTraversable() const = 0;

    // ---------------- Path Reconstruction ----------------
    virtual IGridNode* GetParent() const = 0;
    virtual void SetParent(IGridNode* Parent) = 0;

    // ---------------- Utilities ----------------
    virtual void ResetState() = 0;
    virtual int32 GetNodeID() const = 0;
    virtual void SetTag(FName Tag) = 0;
    virtual FName GetTag() const = 0;
};

//============= GridGraph ============//

class IGridGraph
{
public:
    virtual ~IGridGraph() {}
    
    virtual TArray<IGridNode*> GetAllNodes() = 0;
    virtual IGridNode* FindNodeByID(int32 NodeID) = 0;
    virtual IGridNode* FindNodeByLocation(const FVector& Location) = 0;
    virtual void ResetAllNodes() = 0;
};
