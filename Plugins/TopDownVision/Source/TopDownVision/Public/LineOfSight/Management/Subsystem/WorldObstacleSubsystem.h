// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LineOfSight/VisionData.h"// for enum, tag type

#include "LineOfSight/WorldObstacle/LevelObstacleTileData.h"//FObstacleMaskTile
#include "WorldObstacleSubsystem.generated.h"


//Delegates
DECLARE_MULTICAST_DELEGATE_OneParam(
	FOnObstacleBakeRequested,
	EObstacleBakeRequest);// command enum
// tried to use it in editor function, but binding only happens in the runtime. cannot work

//--> use editor world iteration for the class itself. no need for binding.
// but just in case, keep the delegate


//Log
TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(ObstacleSubsystem, Log, All);

UCLASS()
class TOPDOWNVISION_API UWorldObstacleSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	

	//World Obstacle Tiles

	UFUNCTION(BlueprintCallable, Category="LineOfSight|Obstacle")//!!! this should be called in the editor function!
	void RequestObstacleBake(EObstacleBakeRequest Request);
	
	void RegisterObstacleTile(FObstacleMaskTile NewTile);
	void ClearObstacleTiles();//clear all

	// Initialize tiles from DataAsset
	UFUNCTION(BlueprintCallable, Category="LineOfSight|Obstacle")
	void InitializeTilesFromDataAsset(ULevelObstacleData* TileData);

	// for deletion
	void RemoveTileByTexture(UTexture2D* Texture);
	
	void GetOverlappingTileIndices(
		const FBox2D& QueryBounds,
		//out
		TArray<int32>& OutIndices) const;
	
	//Getter for passing all tiles
	const TArray<FObstacleMaskTile>& GetTiles() const{ return WorldTiles; }

private:
	//internal functions
	void LoadAndInitializeTiles();
	
	//Variables
public:
	//Delegate
	FOnObstacleBakeRequested OnObstacleBakeRequested;// for the delegate from subsystem
	
private:
	
	//Registered Tiles
	UPROPERTY()
	TArray<FObstacleMaskTile> WorldTiles;

	// Path to the plugin-owned obstacle tile data asset
	UPROPERTY(EditDefaultsOnly, Category="LineOfSight|Obstacle")
	FSoftObjectPath LevelObstacleDataPath =
		FSoftObjectPath(TEXT("/Game/Level/LevelData/DA_LevelRequirementsData.DA_LevelRequirementsData"));
	//object pathname here
};
