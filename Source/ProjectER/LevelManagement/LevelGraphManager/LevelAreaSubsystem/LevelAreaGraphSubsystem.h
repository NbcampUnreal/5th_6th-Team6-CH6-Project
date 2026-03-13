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

    /* ---------- Lifecycle ---------- */

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;


    /* ---------- Graph ---------- */

    TMap<int32, LevelAreaNode*> Nodes;


    /* ---------- Hazard State ---------- */

    TMap<int32, EAreaHazardState> ActiveHazardNodes;


    /* ---------- Graph Registration (Helper Actor uses this) ---------- */

    void RegisterNode(LevelAreaNode* Node);
    void UnregisterNode(int32 NodeID);

    LevelAreaNode* GetNode(int32 NodeID) const;
    TArray<LevelAreaNode*> GetAllAreaNodes() const;


    /* ---------- Hazard System ---------- */

    bool GenerateHazardOrder(int32 HazardCount, int32 Seed, TArray<int32>& OutHazardOrder);
    void ApplyHazardNodes(const TArray<int32>& NodeIDs, EAreaHazardState State);
    void ClearHazards();

    //--> BP exposed Gen
    UFUNCTION(BlueprintCallable)
    bool GenerateHazardOrder_BP(
        int32 HazardCount,
        int32 Seed,
        //out
        TArray<int32>& OutHazardOrder,
        FString& ResultLog);


    /* ---------- Player Query ---------- */

    EAreaHazardState GetNodeHazardState(int32 NodeID) const;
    bool IsNodeHazard(int32 NodeID) const;


    /* ---------- IGridGraph ---------- */

    virtual TArray<IGridNode*> GetAllNodes() override;
    virtual IGridNode* FindNodeByID(int32 NodeID) override;
    virtual IGridNode* FindNodeByLocation(const FVector& Location) override;
    virtual void ResetAllNodes() override;


private:

    bool WouldCreateIsland(int32 CandidateID);
    bool IsGraphConnected() const;
    void BFS(LevelAreaNode* StartNode, TSet<int32>& Visited) const;
};