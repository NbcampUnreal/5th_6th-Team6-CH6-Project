#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LevelManagement/Area/LevelAreaNode.h"
#include "LevelManagement/Requirements/LevelAreaData.h"
#include "CPU/Interface/GridNodeInterface.h"
#include "LevelAreaGraphSubsystem.generated.h"

UCLASS()
class PROJECTER_API ULevelAreaGraphSubsystem : public UWorldSubsystem, public IGridGraph
{
    GENERATED_BODY()

public:

    /* ---------- Graph ---------- */

    TMap<int32, LevelAreaNode*> Nodes;


    /* ---------- Hazard State ---------- */

    // Maps NodeID → current hazard state
    TMap<int32, EAreaHazardState> ActiveHazardNodes;


    /* ---------- Graph Building ---------- */

    void RegisterNode(LevelAreaNode* Node);
    void UnregisterNode(int32 NodeID);

    LevelAreaNode* GetNode(int32 NodeID) const;
    TArray<LevelAreaNode*> GetAllAreaNodes() const;


    /* ---------- Hazard System ---------- */

    bool GenerateHazardOrder(int32 HazardCount, TArray<int32>& OutHazardOrder);
    void ApplyHazardNodes(const TArray<int32>& NodeIDs, EAreaHazardState State);
    void ClearHazards();


    /* ---------- Player Query ---------- */

    EAreaHazardState GetNodeHazardState(int32 NodeID) const;
    bool IsNodeHazard(int32 NodeID) const;


    /* ---------- Surface → Node Mapping ---------- */

    void RegisterFloorMaterial(UPhysicalMaterial* Material, int32 NodeID);
    void RegisterFloorMesh(UStaticMesh* Mesh, int32 NodeID);

    void UnregisterFloorMaterial(UPhysicalMaterial* Material);
    void UnregisterFloorMesh(UStaticMesh* Mesh);

    int32 FindNodeByHitResult(const FHitResult& Hit) const;


    /* ---------- IGridGraph ---------- */

    virtual TArray<IGridNode*> GetAllNodes() override;
    virtual IGridNode* FindNodeByID(int32 NodeID) override;
    virtual IGridNode* FindNodeByLocation(const FVector& Location) override;
    virtual void ResetAllNodes() override;


private:

    /* ---------- Surface Maps ---------- */

    UPROPERTY()
    TMap<TObjectPtr<UPhysicalMaterial>, int32> MaterialToNodeID;

    UPROPERTY()
    TMap<TObjectPtr<UStaticMesh>, int32> MeshToNodeID;


    /* ---------- Graph Helpers ---------- */

    bool WouldCreateIsland(int32 CandidateID);
    bool IsGraphConnected() const;
    void BFS(LevelAreaNode* StartNode, TSet<int32>& Visited) const;
};