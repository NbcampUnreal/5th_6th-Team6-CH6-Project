// Fill out your copyright notice in the Description page of Project Settings.

#include "Wrappers/HexGridGenerator.h"
#include "Engine/Texture2D.h"
#include "Rendering/Texture2DResource.h"
#include "Utility/HexGridRangeLibrary.h"
#include "Utility/UtilityLog.h"

// Log
DEFINE_LOG_CATEGORY(GridGeneration);

/*
//---------------------------- Generate Hexagon Grid ----------------------------------//
FHexGrid UHexGridGenerator::GenerateHexagonGrid(int32 Radius)
{
	UE_LOG(GridGeneration, Log,
		TEXT("UHexGridGenerator::GenerateHexagonGrid >> Radius=%d"),
		Radius);

	FHexGrid Grid;
	Grid.GenerateHexagon(Radius);

	UE_LOG(GridGeneration, Log,
		TEXT("UHexGridGenerator::GenerateHexagonGrid >> Generated %d hexes"),
		Grid.Cells.Num());

	return false;
}



//---------------------------- Generate Square Grid --------------------------------------//
FHexGrid UHexGridGenerator::GenerateSquareGrid(
	const FVector& CenterLocation,
	float X_HalfWidth,
	float Y_HalfHeight,
	float HexSize,
	EHexOrientation Orientation,
	float Padding /*= 0.f#1#)
{
	FHexGrid Grid;

	FHexGridLayout Layout;
	Layout.HexSize = HexSize;
	Layout.Orientation = Orientation;
	Layout.Origin = FVector2D(CenterLocation.X, CenterLocation.Y);

	// Hex dimensions with orientation
	float HexWidth = HexSize * (Orientation == EHexOrientation::PointyTop ? FMath::Sqrt(3.f) : 2.f);
	float HexHeight = HexSize * (Orientation == EHexOrientation::PointyTop ? 2.f : FMath::Sqrt(3.f));

	// Apply padding
	HexWidth += Padding;
	HexHeight += Padding;

	// Convert world units to hex coordinates
	int32 QMin = FMath::FloorToInt(-X_HalfWidth / HexWidth);
	int32 QMax = FMath::CeilToInt(X_HalfWidth / HexWidth);
	int32 RMin = FMath::FloorToInt(-Y_HalfHeight / HexHeight);
	int32 RMax = FMath::CeilToInt(Y_HalfHeight / HexHeight);

	for (int32 q = QMin; q <= QMax; q++)
	{
		for (int32 r = RMin; r <= RMax; r++)
		{
			FHexCoord Hex = FHexCoord::FromAxial(q, r);
			Grid.Cells.Add(Hex);
		}
	}

	return Grid;
}

//---------------------------- Apply Circular Mask ------------------------------------//
void UHexGridGenerator::ApplyCircularMask(FHexGrid& Grid, const FHexCoord& Center, int32 Radius)
{
	UE_LOG(GridGeneration, Log,
		TEXT("UHexGridGenerator::ApplyCircularMask >> Center X=%d Y=%d Z=%d Radius=%d"),
		Center.Hex_X, Center.Hex_Y, Center.Hex_Z, Radius);

	TArray<FHexCoord> ToRemove;

	for (const FHexCoord& Hex : Grid.Cells)
	{
		if (Hex.DistanceTo(Center) > Radius)
			ToRemove.Add(Hex);
	}

	for (const FHexCoord& Hex : ToRemove)
		Grid.Cells.Remove(Hex);

	UE_LOG(GridGeneration, Log,
		TEXT("UHexGridGenerator::ApplyCircularMask >> Removed %d hexes, Remaining %d"),
		ToRemove.Num(), Grid.Cells.Num());
}

//---------------------------- Polygon Mask Helper ------------------------------------//
bool IsInsidePolygon(const FVector2D& Point, const TArray<FVector2D>& Polygon)
{
	int32 Intersections = 0;
	int32 NumVerts = Polygon.Num();

	for (int32 i = 0; i < NumVerts; i++)
	{
		const FVector2D& A = Polygon[i];
		const FVector2D& B = Polygon[(i + 1) % NumVerts];

		if (((A.Y > Point.Y) != (B.Y > Point.Y)) &&
			(Point.X < (B.X - A.X) * (Point.Y - A.Y) / (B.Y - A.Y + 0.00001f) + A.X))
		{
			Intersections++;
		}
	}

	return (Intersections % 2) == 1;
}

//---------------------------- Apply Polygon Mask -------------------------------------//
void UHexGridGenerator::ApplyPolygonMask(FHexGrid& Grid, const FHexGridLayout& Layout, const TArray<FVector2D>& Polygon)
{
	UE_LOG(GridGeneration, Log,
		TEXT("UHexGridGenerator::ApplyPolygonMask >> Polygon Vertices: %d"),
		Polygon.Num());

	TArray<FHexCoord> ToRemove;

	for (const FHexCoord& Hex : Grid.Cells)
	{
		FVector2D WorldPos = Layout.HexToWorld(Hex);
		if (!IsInsidePolygon(WorldPos, Polygon))
			ToRemove.Add(Hex);
	}

	for (const FHexCoord& Hex : ToRemove)
		Grid.Cells.Remove(Hex);

	UE_LOG(GridGeneration, Log,
		TEXT("UHexGridGenerator::ApplyPolygonMask >> Removed %d hexes, Remaining %d"),
		ToRemove.Num(), Grid.Cells.Num());
}

//---------------------------- Apply Generic Mask -------------------------------------//
void UHexGridGenerator::ApplyMask(FHexGrid& Grid, FHexMaskFunction MaskFunc)
{
	UE_LOG(GridGeneration, Log,
		TEXT("UHexGridGenerator::ApplyMask >> Applying custom mask"));

	TArray<FHexCoord> ToRemove;

	for (const FHexCoord& Hex : Grid.Cells)
	{
		if (!MaskFunc(Hex))
			ToRemove.Add(Hex);
	}

	for (const FHexCoord& Hex : ToRemove)
		Grid.Cells.Remove(Hex);

	UE_LOG(GridGeneration, Log,
		TEXT("UHexGridGenerator::ApplyMask >> Removed %d hexes, Remaining %d"),
		ToRemove.Num(), Grid.Cells.Num());
}

//---------------------------- Apply Texture Mask -------------------------------------//
void UHexGridGenerator::ApplyTextureMask(FHexGrid& Grid, const FHexGridLayout& Layout, UTexture2D* MaskTexture, float Threshold)
{
	UE_LOG(GridGeneration, Log,
		TEXT("UHexGridGenerator::ApplyTextureMask >> Threshold=%f"),
		Threshold);

	if (!MaskTexture)
	{
		UE_LOG(GridGeneration, Warning,
			TEXT("UHexGridGenerator::ApplyTextureMask >> No MaskTexture provided"));
		return;
	}

#if WITH_EDITOR
	if (!MaskTexture->Source.IsValid())
	{
		UE_LOG(GridGeneration, Warning,
			TEXT("UHexGridGenerator::ApplyTextureMask >> Texture source invalid"));
		return;
	}

	const uint8* PixelData = MaskTexture->Source.LockMip(0);
	const int32 Width = MaskTexture->GetSizeX();
	const int32 Height = MaskTexture->GetSizeY();

	TArray<FHexCoord> ToRemove;

	for (const FHexCoord& Hex : Grid.Cells)
	{
		FVector2D WorldPos = Layout.HexToWorld(Hex);

		// Map world position to UV
		float U = FMath::Clamp((WorldPos.X - Layout.Origin.X) / (Layout.HexSize * Width), 0.f, 1.f);
		float V = FMath::Clamp((WorldPos.Y - Layout.Origin.Y) / (Layout.HexSize * Height), 0.f, 1.f);

		int32 X = FMath::FloorToInt(U * (Width - 1));
		int32 Y = FMath::FloorToInt(V * (Height - 1));

		int32 Index = (Y * Width + X) * 4;
		float Gray = PixelData[Index] / 255.f; // R channel only

		if (Gray < Threshold)
			ToRemove.Add(Hex);
	}

	for (const FHexCoord& Hex : ToRemove)
		Grid.Cells.Remove(Hex);

	MaskTexture->Source.UnlockMip(0);

	UE_LOG(GridGeneration, Log,
		TEXT("UHexGridGenerator::ApplyTextureMask >> Removed %d hexes, Remaining %d"),
		ToRemove.Num(), Grid.Cells.Num());

	//TODO: Find a way to get rasterized texture data from the texture 2d at runtime
#else
	UE_LOG(GridGeneration, Warning, TEXT("UHexGridGenerator::ApplyTextureMask >> Texture mask only works in editor currently."));
#endif
}

//---------------------------- HexGrid to World --------------------------------------//
TArray<FVector2D> UHexGridGenerator::HexGridToWorld(const FHexGrid& Grid, const FHexGridLayout& Layout)
{
	UE_LOG(GridGeneration, Log,
		TEXT("UHexGridGenerator::HexGridToWorld >> Converting %d hexes to world positions"),
		Grid.Cells.Num());

	TArray<FVector2D> WorldPositions;
	WorldPositions.Reserve(Grid.Cells.Num());

	for (const FHexCoord& Hex : Grid.Cells)
	{
		FVector2D Pos = Layout.HexToWorld(Hex);
		WorldPositions.Add(Pos);
		UE_LOG(GridGeneration, Log,
			TEXT("   Hex X=%d Y=%d Z=%d -> World X=%.2f Y=%.2f"),
			Hex.Hex_X, Hex.Hex_Y, Hex.Hex_Z, Pos.X, Pos.Y);
	}

	return WorldPositions;
}

//---------------------------- Get Neighbors -----------------------------------------//
TArray<FHexCoord> UHexGridGenerator::GetNeighbors(const FHexGrid& Grid, const FHexCoord& Hex)
{
	UE_LOG(GridGeneration, Log,
		TEXT("UHexGridGenerator::GetNeighbors >> Hex: X=%d Y=%d Z=%d"),
		Hex.Hex_X, Hex.Hex_Y, Hex.Hex_Z);

	TArray<FHexCoord> Neighbors = Grid.GetNeighbors(Hex);

	UE_LOG(GridGeneration, Log,
		TEXT("UHexGridGenerator::GetNeighbors >> Found %d neighbors"),
		Neighbors.Num());

	for (const FHexCoord& N : Neighbors)
	{
		UE_LOG(GridGeneration, Log,
			TEXT("   Neighbor >> X=%d Y=%d Z=%d"),
			N.Hex_X, N.Hex_Y, N.Hex_Z);
	}

	return Neighbors;
}

//---------------------------- Get Ring ----------------------------------------------//
TArray<FHexCoord> UHexGridGenerator::GetRing(const FHexGrid& Grid, const FHexCoord& Center, int32 Radius)
{
	UE_LOG(GridGeneration, Log,
		TEXT("UHexGridGenerator::GetRing >> Center X=%d Y=%d Z=%d Radius=%d"),
		Center.Hex_X, Center.Hex_Y, Center.Hex_Z, Radius);

	TArray<FHexCoord> Ring = Grid.GetRing(Center, Radius);

	UE_LOG(GridGeneration, Log,
		TEXT("UHexGridGenerator::GetRing >> Found %d hexes in ring"),
		Ring.Num());

	return Ring;
}
*/

bool UHexGridGenerator::GenerateHexagonGrid(const FHexGridLayout& Layout, const FHexCoord& CenterHex, int32 Radius,
	FHexGridWrapper& OutGrid)
{
	OutGrid.Grid.Cells.Empty();

	TArray<FHexCoord> RingHexes;
	TArray<FHexCoord> InnerHexes;

	// Use range library to get coordinates
	bool bSuccess = UHexGridRangeLibrary::GenerateHexRing(Layout, CenterHex, Radius, false, RingHexes, InnerHexes);

	if (!bSuccess)
	{
		UE_LOG(HexGridUtilityLog, Warning, TEXT("HexGridGenerator: Failed to generate hex ring"));
		return false;
	}

	// Combine ring + inner hexes for full area
	OutGrid.Grid.Cells.Append(RingHexes);
	OutGrid.Grid.Cells.Append(InnerHexes);

	UE_LOG(HexGridUtilityLog, Log, TEXT("HexGridGenerator: Generated hex grid with %d hexes"), OutGrid.Grid.Cells.Num());

	return true;
}
