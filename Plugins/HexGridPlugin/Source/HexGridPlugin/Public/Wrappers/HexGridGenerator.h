#pragma once

#include "CoreMinimal.h"
#include "Hex/HexagonData.h"

#include "HexGridGenerator.generated.h"

//Forward
class UTexture2D;


//Log
HEXGRIDPLUGIN_API DECLARE_LOG_CATEGORY_EXTERN(GridGeneration, Log, All);


UCLASS(Blueprintable)
class HEXGRIDPLUGIN_API UHexGridGenerator : public UObject
{
	GENERATED_BODY()

public:
	//Test
	/** Generate a basic hex grid of given radius (centered at origin) */
	UFUNCTION(BlueprintCallable, Category="HexGrid|Generator")
	static bool GenerateHexagonGrid(
		const FHexGridLayout& Layout,
		const FHexCoord& CenterHex,
		int32 Radius,
		// out
		FHexGridWrapper& OutGrid);



	
	//====== Grid Generation ================//
	
	/*
	// default hex shape population
	UFUNCTION(BlueprintCallable, Category = "HexGrid")
	static bool GenerateHexagonGrid(
		float Radius,// distance from center to outer hex vertex
		const FVector& CenterLocation,
		float DesirableCellHexRadius,
		EHexOrientation Orientation,
		bool FitToEdge,
		//out
		FHexGridWrapper& GridWrapper);

	//Circular area which uses hexagongrid generation as internal and trim out for making circular area
	UFUNCTION(BlueprintCallable, Category="HexGrid")
	static bool GenerateCircularGrid(
		float Radius,// world radius
		const FVector& Center,
		float DesirableCellHexRadius,
		EHexOrientation Orientation,
		bool FitToEdge,
		//out
		FHexGridWrapper& OutGrid);

	UFUNCTION(BlueprintCallable, Category = "HexGrid")
	static bool GenerateSquareGrid(
		const FVector& CenterLocation,
		float X_HalfWidth,
		float Y_HalfWidth,
		float DesirableCellHexRadius,
		EHexOrientation Orientation,
		bool FitToEdge,
		//out
		FHexGridWrapper& GridWrapper,
		float Padding = 0.f);//default 0 for padding. padding will be useful for masking base purpose

	//====== Masking =========================//

	// ===== Masking (post-processing) =====
	static bool ApplyPolygonMask(FHexGridWrapper& GridWrapper, const TArray<FVector2D>& PolygonPoints);
	static bool ApplyTextureMask(FHexGridWrapper& GridWrapper, UTexture2D* MaskTexture, float Threshold);
	static bool ApplyCustomMask(FHexGridWrapper& GridWrapper, TFunction<bool(const FHexCoord&)> MaskFunc);


	//======= Helper functions ==========//
	
	UFUNCTION(BlueprintCallable, Category = "HexGrid")
	static bool GetNeighbors(
		const FHexGrid& Grid,
		const FHexCoord& Hex,
		//out
		TArray<FHexCoord>& Neighbors);

	UFUNCTION(BlueprintCallable, Category = "HexGrid")
	static TArray<FHexCoord> GetRing(
		const FHexGrid& Grid,
		const FHexCoord& Center,
		int32 Radius,
		bool bFloodFill,
		//out
		TArray<FHexCoord>& RingCoords,
		TArray<FHexCoord>& CoordsInRange);

	// Check if a set of hexes forms a continuous ring for lasso
	UFUNCTION(BlueprintCallable, Category="HexGrid")
	static bool IsUsableAsLasso(
		const TArray<FHexCoord>& HexSet,
		const FHexGrid& Grid);

	UFUNCTION(BlueprintCallable, Category="HexGrid")
	static bool GenerateLassoFromPoints(
		TArray<FVector2D> PointsWorldLocations,
		TArray<FHexCoord>& LassoHexSet);

	// Get all hexes inside a lasso/polygon selection
	UFUNCTION(BlueprintCallable, Category="HexGrid")
	static bool GetCoordsInLassoRange(
		const FHexGrid& Grid,
		const FHexGridLayout& Layout,
		const TArray<FVector2D>& PolygonPoints,
		bool bFloodFill,
		//out
		TArray<FHexCoord> LassoHexCoords,
		TArray<FHexCoord>& CoordsInRange);*/

	
};