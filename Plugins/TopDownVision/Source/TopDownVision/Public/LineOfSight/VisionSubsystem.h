// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LineOfSight/VisionData.h"// for enum, tag type
#include "WorldObstacle/WorldObstacleData.h"
#include "LineOfSight/WorldObstacle/ObstacleTileData.h"//FObstacleMaskTile
#include "VisionSubsystem.generated.h"


//forward declare
class ULineOfSightComponent;// the source of the texture

USTRUCT()
struct FRegisteredProviders
{
	GENERATED_BODY()
	
	UPROPERTY()
	TArray<ULineOfSightComponent*> RegisteredList;
};


//Delegates
DECLARE_MULTICAST_DELEGATE_OneParam(
	FOnObstacleBakeRequested,
	EObstacleBakeRequest);// command enum
// tried to use it in editor function, but binding only happens in the runtime. cannot work

//--> use editor world iteration for the class itself. no need for binding.
// but just in case, keep the delegate


//Log
TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(VisionSubsystem, Log, All);

UCLASS()
class TOPDOWNVISION_API UVisionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	

	//Registration
	UFUNCTION(BlueprintCallable, Category="LineOfSight")
	bool RegisterProvider(ULineOfSightComponent* Provider, EVisionChannel InVisionChannel);
	UFUNCTION(BlueprintCallable, Category="LineOfSight")
	void UnregisterProvider(ULineOfSightComponent* Provider, EVisionChannel InVisionChannel);

	//World Obstacle Tiles

	UFUNCTION(BlueprintCallable, Category="LineOfSight|Obstacle")//!!! this should be called in the editor function!
	void RequestObstacleBake(EObstacleBakeRequest Request);
	
	void RegisterObstacleTile(FObstacleMaskTile NewTile);
	void ClearObstacleTiles();//clear all

	// Initialize tiles from DataAsset
	UFUNCTION(BlueprintCallable, Category="LineOfSight|Obstacle")
	void InitializeTilesFromDataAsset(UObstacleTileData* TileData);

	// for deletion
	void RemoveTileByTexture(UTexture2D* Texture);
	
	void GetOverlappingTileIndices(
		const FBox2D& QueryBounds,
		//out
		TArray<int32>& OutIndices) const;
	
	// getter of same team+shared vision
	TArray<ULineOfSightComponent*> GetProvidersForTeam(EVisionChannel TeamChannel) const;

	//Getter for passing all tiles
	const TArray<FObstacleMaskTile>& GetTiles() const{ return WorldTiles; }


//Variables
	//Delegate
	FOnObstacleBakeRequested OnObstacleBakeRequested;// for the delegate from subsystem
	
private:
	// Registered actor-local LOS providers
	UPROPERTY()
	TMap<EVisionChannel, FRegisteredProviders> VisionMap;

	//Registered Tiles
	UPROPERTY()
	TArray<FObstacleMaskTile> WorldTiles;
	
	// Data asset reference to load baked tiles at runtime
	UPROPERTY(EditDefaultsOnly, Category="LineOfSight|Obstacle")
	UObstacleTileData* DefaultObstacleTileData;
	// path to the data asset
	const FString AssetSavePath=TEXT("/Game/PSM/WorldBaker/DA_WorldTileData.DA_WorldTileData");
	
};
