// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ObstacleData.generated.h"
/**
 * 
 */
//Package Data for later usage
USTRUCT(BlueprintType)
struct FObstacleMaskTile
{
	GENERATED_BODY()
	
	UPROPERTY()
	UTexture2D* Mask;
	
	UPROPERTY()
	FBox2D WorldBounds;
	
	UPROPERTY()
	FVector2D WorldSize;
	
	UPROPERTY()
	FMatrix WorldToUV;// pre computed value for Local RT merging
};

//Tile Texture baking command
UENUM(BlueprintType)
enum class EObstacleBakeRequest : uint8
{
	None UMETA(DisplayName = "None"),//invalid
	
	Clear UMETA(DisplayName = "Clear"),
	Bake UMETA(DisplayName = "Bake"),
	Rebuild UMETA(DisplayName ="Rebuild"),// Clear + Bake
};


/**
 *  This is for linking the Editor world baked data to the runtime world
 *  Obstacle maks data will be transferred by this data asset
 */
UCLASS(Blueprintable)
class TOPDOWNVISION_API ULevelObstacleData : public UDataAsset
{
	GENERATED_BODY()

public:
	
	// All baked tiles
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Obstacle Tiles")
	TArray<FObstacleMaskTile> Tiles;


	// other required data in here
};