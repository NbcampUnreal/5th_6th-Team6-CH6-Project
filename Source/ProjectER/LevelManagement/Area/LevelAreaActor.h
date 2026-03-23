#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelManagement/Requirements/LevelAreaData.h"
#include "LevelManagement/Requirements/LevelAreaPhysicalMaterial.h"
#include "LevelManagement/Requirements/LevelAreaGraphData.h"
#include "LevelAreaActor.generated.h"

UCLASS()
class PROJECTER_API ALevelAreaActor : public AActor
{
	GENERATED_BODY()

public:

	ALevelAreaActor();

#pragma region NodeBaking

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area")
	int32 NodeID = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area")
	TArray<TObjectPtr<ALevelAreaActor>> NeighborActors;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area|Surface")
	TObjectPtr<ULevelAreaPhysicalMaterial> FloorMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area|Surface")
	TEnumAsByte<ECollisionChannel> FloorTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area|Bake")
	TObjectPtr<ULevelAreaGraphData> GraphData = nullptr;

	UFUNCTION(CallInEditor, Category="Area|Surface")
	void ApplyMaterialToMeshes();

	UFUNCTION(CallInEditor, Category="Area|Bake")
	void BakeAllNodes();

	UFUNCTION(CallInEditor, Category="Area|Bake")
	void ClearGraphData();

private:

	void BakeNode();

#pragma endregion
};