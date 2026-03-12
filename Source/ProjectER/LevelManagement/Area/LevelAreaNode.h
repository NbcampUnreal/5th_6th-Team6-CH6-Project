#pragma once

#include "CoreMinimal.h"
#include "CPU/Interface/GridNodeInterface.h"

/*
 * Represents a dungeon room (level area).
 * Rooms are connected through bridges / corridors.
 */

class PROJECTER_API LevelAreaNode : public IGridNode
{
public:
    
    /* ================================
       Bridge / Connection Definition
    ================================= */

    struct FBridgeConnection
    {
        int32 TargetNodeID = INDEX_NONE;

        FVector BridgeStart;   // exit point in this room
        FVector BridgeEnd;     // entry point in neighbor room
    };


private:

    /* -------- Node Identity -------- */

    int32 NodeID = INDEX_NONE;

    FVector RoomCenter = FVector::ZeroVector;


    /* -------- Room Shape -------- */

    FVector RoomExtent = FVector(500,500,200); // size of the room


    /* -------- Graph Connections -------- */

    TArray<FBridgeConnection> Connections;


    /* -------- Pathfinding State -------- */

    float Cost = 0.f;
    float HeuristicCost = 0.f;

    uint8 Flags = 0;

    IGridNode* ParentNode = nullptr;

    FName Tag;


public:

    /* ================================
       Setup
    ================================= */

    void SetNodeID(int32 ID) { NodeID = ID; }

    void SetRoomCenter(const FVector& Center)
    {
        RoomCenter = Center;
    }

    void SetRoomExtent(const FVector& Extent)
    {
        RoomExtent = Extent;
    }


    void AddConnection(
        int32 TargetID,
        const FVector& BridgeStart,
        const FVector& BridgeEnd)
    {
        FBridgeConnection NewConnection;

        NewConnection.TargetNodeID = TargetID;
        NewConnection.BridgeStart = BridgeStart;
        NewConnection.BridgeEnd = BridgeEnd;

        Connections.Add(NewConnection);
    }


    /* ================================
       Graph Interface
    ================================= */

    virtual int32 GetNumNeighbors() const override
    {
        return Connections.Num();
    }

    virtual IGridNode* GetNeighborPointerGraph(
        int32 Index,
        IGridGraph* Graph) const override;

    virtual IGridNode* GetNeighborIndexGraph(
        int32 Index,
        IGridGraph* Graph) const override;


    /* ================================
       Spatial
    ================================= */

    virtual FVector GetWorldLocation() const override
    {
        return RoomCenter;
    }

    const FVector& GetRoomExtent() const
    {
        return RoomExtent;
    }


    const FBridgeConnection* GetConnection(int32 Index) const
    {
        return Connections.IsValidIndex(Index) ? &Connections[Index] : nullptr;
    }


    /* ================================
       Pathfinding
    ================================= */

    virtual float GetCostTo(const IGridNode* TargetNode) const override;

    virtual void SetCost(float InCost) override
    {
        Cost = InCost;
    }

    virtual float GetHeuristicCost(const IGridNode* TargetNode) const override;

    virtual void SetHeuristicCost(float InCost) override
    {
        HeuristicCost = InCost;
    }


    /* ================================
       Flags
    ================================= */

    virtual bool HasFlag(uint8 Flag) const override
    {
        return (Flags & Flag) != 0;
    }

    virtual void SetFlag(uint8 Flag, bool bValue) override
    {
        if (bValue)
            Flags |= Flag;
        else
            Flags &= ~Flag;
    }

    virtual bool IsTraversable() const override
    {
        return !HasFlag(1 << 0); // blocked / hazard
    }


    /* ================================
       Parent
    ================================= */

    virtual IGridNode* GetParent() const override
    {
        return ParentNode;
    }

    virtual void SetParent(IGridNode* Parent) override
    {
        ParentNode = Parent;
    }


    /* ================================
       Utilities
    ================================= */

    virtual void ResetState() override
    {
        Cost = 0;
        HeuristicCost = 0;
        ParentNode = nullptr;
        Flags = 0;
    }

    virtual int32 GetNodeID() const override
    {
        return NodeID;
    }

    virtual void SetTag(FName InTag) override
    {
        Tag = InTag;
    }

    virtual FName GetTag() const override
    {
        return Tag;
    }
};