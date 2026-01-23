// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Hex/HexagonData.h"
#include "HexGridRangeLibrary.generated.h"

//forward
class UTexture2D;

/**
 *  this is for 
 */

USTRUCT(BlueprintType)// for texture island case. make wrapper struct so that it can be in the tarray as well
struct FHexCoordArray
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FHexCoord> Hexes;
};

UCLASS()
class HEXGRIDPLUGIN_API UHexGridRangeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	
	UFUNCTION(BlueprintCallable, Category="HexGrid|Ring")//basic hex shape ring generation
	static bool GenerateHexRing(
		const FHexGridLayout& Layout,
		const FHexCoord& CenterHex,
		int32 Radius,
		bool bJustRing,
		//output
		TArray<FHexCoord>& OutRingHexes,
		TArray<FHexCoord>& OutInnerHexes);

	UFUNCTION(BlueprintCallable, Category="HexGrid|Ring")//circular ring generation
	static bool GenerateCircleRing(
		const FHexGridLayout& Layout,
		const FVector& CenterLocation,
		float Radius,
		bool bJustRing,
		//output
		TArray<FHexCoord>& OutRingHexes,
		TArray<FHexCoord>& OutInnerHexes);

	UFUNCTION(BlueprintCallable, Category="HexGrid|Ring")
	static bool GenerateLassoRing(
		const FHexGridLayout& Layout,
		const TArray<FVector2D>& PolygonPoints,
		bool bJustRing,
		// output
		TArray<FHexCoord>& OutRingHexes,
		TArray<FHexCoord>& OutInnerHexes
	);

	// this is mostly for canvas grid for texture masking.
	UFUNCTION(BlueprintCallable, Category="HexGrid|Range")
	static bool GenerateQuadRange(
		const FHexGridLayout& Layout,
		const FVector2D& Center,
		float Width,
		float Height,
		float RotationDegrees,
		bool bJustRing,
		// out
		TArray<FHexCoord>& OutInnerHexes,
		TArray<FHexCoord>& OutRingHexes
	);
	
	// Texture masking is more for post modification of the generated grid, this doesnt belong in here.
	
	/*UFUNCTION(BlueprintCallable, Category="HexGrid|Mask")
	static bool GenerateTextureMask(
		const FHexGridWrapper& GridWrapper,
		const UTexture2D* MaskTexture,
		FIntVector2 PixelResolution,
		float ThresholdAlpha,
		FVector2D Offset,
		float TextureRotation,
		FVector2D TextureScale,
		 // 0~1, any pixel brighter than alpha is included
		//bool bJustRing,
		//--> the area will be calculated to get the ring anyway, so no need to use it. if area is not needed, then just dont use it
		//output
		TArray<FHexCoordArray>& OutRingHexes,// for texture there can be islands. so it need to return more than one results
		TArray<FHexCoordArray>& OutSelectedHexes);*/
};
