#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LevelManagement/Area/LevelAreaNode.h"
#include "LevelAreaGraphSubsystem.generated.h"


UCLASS()
class PROJECTER_API ULevelAreaGraphSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	/* ---------- Graph ---------- */

	TMap<int32, LevelAreaNode*> Nodes;


	/* ---------- Graph Building ---------- */

	void RegisterNode(LevelAreaNode* Node);

	LevelAreaNode* GetNode(int32 NodeID) const;

	TArray<LevelAreaNode*> GetAllNodes() const;


	/* ---------- Hazard System ---------- */

	bool GenerateHazardOrder(
		int32 HazardCount,
		TArray<int32>& OutHazardOrder);


private:
	//helper for finding hazard area order

	bool WouldCreateIsland(int32 CandidateID);

	bool IsGraphConnected() const;

	void BFS(LevelAreaNode* StartNode, TSet<int32>& Visited) const;
};