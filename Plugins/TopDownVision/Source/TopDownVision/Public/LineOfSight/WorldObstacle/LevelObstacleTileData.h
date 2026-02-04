// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LineOfSight/WorldObstacle/ObstacleData.h"//Tile data
#include "LevelObstacleTileData.generated.h"


UCLASS(Blueprintable)
class TOPDOWNVISION_API UWorldRequirementList : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Obstacle")
	TMap<FString/*world name*/, TObjectPtr<ULevelObstacleData>> WorldRequirements;
};
