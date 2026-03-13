#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelManagement/Requirements/LevelAreaPhysicalMaterial.h"
#include "LevelManagement/Requirements/LevelAreaGraphData.h"
#include "LevelAreaActor.generated.h"

UCLASS()
class PROJECTER_API ALevelAreaActor : public AActor
{
	GENERATED_BODY()

public:

	ALevelAreaActor();

	/* ---------- Config ---------- */

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area")
	int32 NodeID = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area")
	TArray<TObjectPtr<ALevelAreaActor>> NeighborActors;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area|Surface")
	TObjectPtr<ULevelAreaPhysicalMaterial> FloorMaterial = nullptr;

	// set the trace channel for area id checker
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area|Surface")
	TEnumAsByte<ECollisionChannel> FloorTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area|Bake")
	TObjectPtr<ULevelAreaGraphData> GraphData = nullptr;


	/* ---------- Editor Utility ---------- */

	UFUNCTION(CallInEditor, Category="Area|Surface")
	void ApplyMaterialToMeshes();

	// Bakes all ALevelAreaActors in the level into their GraphData assets
	UFUNCTION(CallInEditor, Category="Area|Bake")
	void BakeAllNodes();

	// Clears all node records from the GraphData asset
	UFUNCTION(CallInEditor, Category="Area|Bake")
	void ClearGraphData();


private:

	// Bakes this single node into GraphData — called internally by BakeAllNodes
	void BakeNode();
};