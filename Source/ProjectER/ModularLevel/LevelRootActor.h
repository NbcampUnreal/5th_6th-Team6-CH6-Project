#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelData/LevelExtractionData.h"
#include "LevelRootActor.generated.h"

//Log
PROJECTER_API DECLARE_LOG_CATEGORY_EXTERN(LevelExtraction, Log, All);

UCLASS()
class PROJECTER_API ALevelRootActor : public AActor
{
	GENERATED_BODY()

public:

	ALevelRootActor();
	
	
	// Collect actors attached to this root
	UFUNCTION(BlueprintCallable, Category="Level Extraction")
	void ExtractActorsToAsset();

	// Spawn actors from the asset
	UFUNCTION(BlueprintCallable, Category="Level Extraction")
	void LoadActorsFromAsset();

	// Clear stored data
	UFUNCTION(BlueprintCallable, Category="Level Extraction")
	void ClearAssetActors();

	// Update asset from current hierarchy
	UFUNCTION(BlueprintCallable, Category="Level Extraction")
	void UpdateAssetActors();

private:

	bool IsValidToRun(UWorld*& OutWorld);



protected:
	
	// Asset where extracted actors will be stored
	UPROPERTY(EditAnywhere, Category="Level Extraction")
	TObjectPtr<ULevelExtractionData> TargetAsset;

	UPROPERTY(EditAnywhere, Category="Level Extraction")
	TObjectPtr<UStaticMeshComponent> IndicatorMesh;// the visual mesh for the root
};