// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LineOfSight/WorldObstacle/WorldObstacleData.h"//Tile data
#include "ObstacleTileData.generated.h"

/**
 *  This is for linking the Editor world baked data to the runtime world
 *  Obstacle maks data will be transferred by this data asset
 */
UCLASS(Blueprintable)
class TOPDOWNVISION_API UObstacleTileData : public UDataAsset
{
	GENERATED_BODY()

public:

	// All baked tiles
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Obstacle Tiles")
	TArray<FObstacleMaskTile> Tiles;
};
