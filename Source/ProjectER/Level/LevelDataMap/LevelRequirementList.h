// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LineOfSight/WorldObstacle/LevelObstacleTileData.h"//tile data

#include "LevelRequirementList.generated.h"

/**
 *  this will have the data matchign with the level
 *
 *  currently, the obstacle data is saved per level so that it can be loaded into the initializing vision-subsystem correctly
 */


USTRUCT(BlueprintType)
struct FLevelRequirements
{
	GENERATED_BODY()

	// put required data per level in here
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Obstacle")
	TObjectPtr<ULevelObstacleData> ObstacleDataAsset = nullptr;
};


UCLASS(BlueprintType)
class PROJECTER_API ULevelRequirementList : public UDataAsset
{
	GENERATED_BODY()
	
public:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Level Data")
	TMap<FName/*level name here*/,FLevelRequirements > LevelData;
};
