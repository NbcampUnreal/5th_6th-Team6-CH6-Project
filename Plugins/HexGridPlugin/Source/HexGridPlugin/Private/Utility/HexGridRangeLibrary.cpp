// Fill out your copyright notice in the Description page of Project Settings.


#include "Utility/HexGridRangeLibrary.h"
#include "Engine/Texture2D.h"
#include "Engine/Texture.h"// for texture range
#include "Engine/TextureRenderTarget2D.h"
#include "Math/UnrealMathUtility.h"
#include "Containers/Queue.h"
#include "Utility/HexGridPathLibrary.h"


#include "Utility/UtilityLog.h"//log


//======================= Hex Ring ===================================================================================//
#pragma region DefaultHexRing

bool UHexGridRangeLibrary::GenerateHexRing(const FHexGridLayout& Layout, const FHexCoord& CenterHex, int32 Radius,
	bool bJustRing, TArray<FHexCoord>& OutRingHexes, TArray<FHexCoord>& OutInnerHexes)
{
	OutRingHexes.Empty();
	OutInnerHexes.Empty();

	if (Radius < 0)
		return false;

	for (int32 x = -Radius; x <= Radius; ++x)
	{
		for (int32 y = FMath::Max(-Radius, -x - Radius);
			 y <= FMath::Min(Radius, -x + Radius);
			 ++y)
		{
			int32 z = -x - y;
			FHexCoord Hex = CenterHex + FHexCoord(x, y, z);

			const int32 Dist = CenterHex.DistanceTo(Hex);

			if (Dist == Radius)
			{
				OutRingHexes.Add(Hex);
			}
			else if (!bJustRing && Dist < Radius)
			{
				OutInnerHexes.Add(Hex);
			}
		}
	}

	return OutRingHexes.Num() > 0;
}

#pragma endregion

//======================= Circular Ring ==============================================================================//
#pragma region Circular Generation

bool UHexGridRangeLibrary::GenerateCircleRing(const FHexGridLayout& Layout, const FVector& CenterLocation, float Radius,
                                              bool bJustRing, TArray<FHexCoord>& OutRingHexes, TArray<FHexCoord>& OutInnerHexes)
{
	OutRingHexes.Empty();
	OutInnerHexes.Empty();

	const FVector2D Center2D(CenterLocation.X, CenterLocation.Y);
	const float HexRadius = Layout.HexSize * 0.5f;

	// Estimate hex bounds
	const int32 MaxSteps = FMath::CeilToInt(Radius / Layout.HexSize) + 2;

	FHexCoord CenterHex = Layout.WorldToHex(Center2D);

	for (int32 q = -MaxSteps; q <= MaxSteps; ++q)
	{
		for (int32 r = -MaxSteps; r <= MaxSteps; ++r)
		{
			FHexCoord Hex = CenterHex + FHexCoord(q, r, -q - r);
			FVector2D HexPos = Layout.HexToWorld(Hex);

			float Dist = FVector2D::Distance(HexPos, Center2D);

			if (FMath::Abs(Dist - Radius) <= HexRadius)
			{
				OutRingHexes.Add(Hex);
			}
			else if (!bJustRing && Dist < Radius)
			{
				OutInnerHexes.Add(Hex);
			}
		}
	}

	return OutRingHexes.Num() > 0;
}
#pragma endregion

//======================= Lasso Ring =================================================================================//
#pragma region LassoRing Generation

//GenerateLassoRing --> Internal functions
	/** Returns true if Point is inside polygon (ray-casting algorithm) */
bool IsPointInsidePolygon(const FVector2D& Point, const TArray<FVector2D>& Polygon)
{
	int32 Crossings = 0;
	int32 NumPoints = Polygon.Num();
	for (int32 i = 0; i < NumPoints; ++i)
	{
		const FVector2D& A = Polygon[i];
		const FVector2D& B = Polygon[(i + 1) % NumPoints];

		if (((A.Y <= Point.Y) && (B.Y > Point.Y)) || ((A.Y > Point.Y) && (B.Y <= Point.Y)))
		{
			float Slope = (Point.Y - A.Y) / (B.Y - A.Y);
			float XIntersect = A.X + Slope * (B.X - A.X);
			if (Point.X < XIntersect)
				Crossings++;
		}
	}
	return (Crossings % 2) == 1;
}

	/** Returns shortest distance from Point to polygon edges */
float DistanceToPolygonEdges(const FVector2D& Point, const TArray<FVector2D>& Polygon)
{
	float MinDist = FLT_MAX;
	int32 NumPoints = Polygon.Num();
	for (int32 i = 0; i < NumPoints; ++i)
	{
		const FVector2D& A = Polygon[i];
		const FVector2D& B = Polygon[(i + 1) % NumPoints];

		FVector2D AB = B - A;
		float T = FMath::Clamp(FVector2D::DotProduct(Point - A, AB) / AB.SizeSquared(), 0.f, 1.f);
		FVector2D Closest = A + T * AB;
		float Dist = FVector2D::Distance(Point, Closest);
		MinDist = FMath::Min(MinDist, Dist);
	}
	return MinDist;
}

//!!!! Now using path finding for connecting lasso ring
bool UHexGridRangeLibrary::GenerateLassoRing( const FHexGridLayout& Layout,const TArray<FVector2D>& PolygonPoints,
	bool bJustRing, TArray<FHexCoord>& OutRingHexes, TArray<FHexCoord>& OutInnerHexes)
{
	OutRingHexes.Empty();
    OutInnerHexes.Empty();

    if (PolygonPoints.Num() < 3)
    {
        UE_LOG(HexGridUtilityLog, Warning,
            TEXT("GenerateLassoRing >> Polygon needs at least 3 points"));
        return false;
    }
	
    // 1. Compute polygon AABB (world space)
    FVector2D Min(FLT_MAX, FLT_MAX);
    FVector2D Max(-FLT_MAX, -FLT_MAX);

    for (const FVector2D& P : PolygonPoints)
    {
        Min.X = FMath::Min(Min.X, P.X);
        Min.Y = FMath::Min(Min.Y, P.Y);
        Max.X = FMath::Max(Max.X, P.X);
        Max.Y = FMath::Max(Max.Y, P.Y);
    }
	
    // 2. Convert AABB → hex bounds
    FHexCoord MinHex = Layout.WorldToHex(Min);
    FHexCoord MaxHex = Layout.WorldToHex(Max);

    // Expand slightly to be safe
    const int32 Padding = 2;
	
    // 3. Scan hexes inside bounding region
    TSet<FHexCoord> InsideSet;
    TSet<FHexCoord> RingSet;

    const float HexRadius = Layout.HexSize * 0.5f;

    for (int32 q = MinHex.Hex_X - Padding; q <= MaxHex.Hex_X + Padding; ++q)
    {
        for (int32 r = MinHex.Hex_Y - Padding; r <= MaxHex.Hex_Y + Padding; ++r)
        {
            FHexCoord Hex(q, r, -q - r);
            FVector2D HexWorld = Layout.HexToWorld(Hex);

            const bool bInside =
                IsPointInsidePolygon(HexWorld, PolygonPoints);

            if (!bInside)
                continue;

            InsideSet.Add(Hex);

            // Check if this hex is near polygon boundary → ring
            const float DistToEdge =
                DistanceToPolygonEdges(HexWorld, PolygonPoints);

            if (DistToEdge <= HexRadius)
            {
                RingSet.Add(Hex);
            }
        }
    }
	
    // 4. Output results
    OutRingHexes = RingSet.Array();

    if (!bJustRing)
    {
        for (const FHexCoord& H : InsideSet)
        {
            if (!RingSet.Contains(H))
            {
                OutInnerHexes.Add(H);
            }
        }
    }

    UE_LOG(HexGridUtilityLog, Log,
        TEXT("GenerateLassoRing >> Ring %d | Inner %d"),
        OutRingHexes.Num(), OutInnerHexes.Num());

    return OutRingHexes.Num() > 0;
}
#pragma endregion

//======================= Quad Range =================================================================================//
#pragma region Quad Range

bool UHexGridRangeLibrary::GenerateQuadRange(const FHexGridLayout& Layout, const FVector2D& Center, float Width,
	float Height, float RotationDegrees, bool bJustRing, TArray<FHexCoord>& OutInnerHexes,
	TArray<FHexCoord>& OutRingHexes)
{
	OutInnerHexes.Empty();
	OutRingHexes.Empty();

	//center offset
	const float HalfW = Width * 0.5f;
	const float HalfH = Height * 0.5f;

	// Precompute rotation (inverse)
	const float Rad = FMath::DegreesToRadians(-RotationDegrees);
	const float CosR = FMath::Cos(Rad);
	const float SinR = FMath::Sin(Rad);

	// Conservative scan radius
	const float MaxExtent = FMath::Max(HalfW, HalfH);
	const int32 ScanRadius =
		FMath::CeilToInt(MaxExtent / Layout.HexSize) + 2;

	const FHexCoord CenterHex = Layout.WorldToHex(Center);

	for (int32 dx = -ScanRadius; dx <= ScanRadius; ++dx)
	{
		for (int32 dy = -ScanRadius; dy <= ScanRadius; ++dy)
		{
			FHexCoord Hex = CenterHex + FHexCoord::FromAxial(dx, dy);
			FVector2D World = Layout.HexToWorld(Hex);

			// Transform into rectangle local space
			FVector2D Local = World - Center;

			FVector2D Rotated;
			Rotated.X = Local.X * CosR - Local.Y * SinR;
			Rotated.Y = Local.X * SinR + Local.Y * CosR;

			const bool bInside =
				FMath::Abs(Rotated.X) <= HalfW &&
				FMath::Abs(Rotated.Y) <= HalfH;

			if (!bInside)
				continue;

			// Distance to nearest edge
			const float DistX = HalfW - FMath::Abs(Rotated.X);
			const float DistY = HalfH - FMath::Abs(Rotated.Y);
			const float EdgeDist = FMath::Min(DistX, DistY);

			const bool bIsRing =
				EdgeDist <= Layout.HexSize * 0.75f;

			if (bIsRing)
			{
				OutRingHexes.Add(Hex);
			}
			else if (!bJustRing)
			{
				OutInnerHexes.Add(Hex);
			}
		}
	}

	return OutInnerHexes.Num() > 0 || OutRingHexes.Num() > 0;
}
#pragma endregion


//======================= Texture Ring =================================================================================//
#pragma region TextureRing Generation

/*bool UHexGridRangeLibrary::GenerateTextureMask(const FHexGridWrapper& GridWrapper, const UTexture2D* MaskTexture,
	FIntVector2 PixelResolution, float ThresholdAlpha, FVector2D TextureOffset, float TextureRotation, FVector2D TextureScale ,
	TArray<FHexCoordArray>& OutRingHexes, TArray<FHexCoordArray>& OutSelectedHexes)
{
	OutRingHexes.Empty();
    OutSelectedHexes.Empty();

    if (!MaskTexture || !MaskTexture->GetPlatformData() || MaskTexture->GetPlatformData()->Mips.Num() == 0)
    {
        UE_LOG(HexGridUtilityLog, Error, TEXT("GenerateTextureMaskFloodFill: Invalid texture"));
        return false;
    }

    const FTexture2DMipMap& Mip = MaskTexture->GetPlatformData()->Mips[0];
    const int32 TexWidth = PixelResolution.X > 0 ? PixelResolution.X : Mip.SizeX;
    const int32 TexHeight = PixelResolution.Y > 0 ? PixelResolution.Y : Mip.SizeY;

    const FByteBulkData* RawData = &Mip.BulkData;
    const uint8* PixelData = static_cast<const uint8*>(RawData->LockReadOnly());
    if (!PixelData)
    {
        UE_LOG(HexGridUtilityLog, Error, TEXT("GenerateTextureMaskFloodFill: Failed to lock texture data"));
        return false;
    }

    // Precompute rotation
    const float Rad = FMath::DegreesToRadians(TextureRotation);
    const float CosR = FMath::Cos(Rad);
    const float SinR = FMath::Sin(Rad);

    // Map hex -> is selected
    TMap<FHexCoord, bool> HexSelected;

    for (const FHexCoord& Hex : GridWrapper.Grid.Cells)
    {
        FVector2D Pos = GridWrapper.GetWorldPos(Hex);

        // Apply offset, scale
        FVector2D Local = (Pos + TextureOffset) * TextureScale;

        // Apply rotation around origin
        FVector2D Rotated;
        Rotated.X = Local.X * CosR - Local.Y * SinR;
        Rotated.Y = Local.X * SinR + Local.Y * CosR;

        // Convert to pixel coordinates
        float U = FMath::Clamp(Rotated.X / TexWidth, 0.f, 1.f);
        float V = FMath::Clamp(Rotated.Y / TexHeight, 0.f, 1.f);
        int32 X = FMath::Clamp(FMath::FloorToInt(U * TexWidth), 0, TexWidth - 1);
        int32 Y = FMath::Clamp(FMath::FloorToInt(V * TexHeight), 0, TexHeight - 1);

        int32 PixelIndex = (Y * TexWidth + X) * 4; // RGBA8
        float Alpha = PixelData[PixelIndex + 3] / 255.f;

        HexSelected.Add(Hex, Alpha >= ThresholdAlpha);
    }

    RawData->Unlock();

    // Flood-fill to find islands
    TSet<FHexCoord> Visited;
    for (const auto& Pair : HexSelected)
    {
        const FHexCoord& Hex = Pair.Key;
        if (!Pair.Value || Visited.Contains(Hex))
            continue;

        // BFS/Flood fill
        TArray<FHexCoord> IslandQueue;
        IslandQueue.Add(Hex);
        TSet<FHexCoord> IslandSet;
        IslandSet.Add(Hex);
        Visited.Add(Hex);

        int32 QueueIndex = 0;
        while (QueueIndex < IslandQueue.Num())
        {
            FHexCoord Current = IslandQueue[QueueIndex++];
            for (const FHexCoord& Neighbor : GridWrapper.Grid.GetNeighbors(Current))
            {
                if (HexSelected.FindRef(Neighbor) && !Visited.Contains(Neighbor))
                {
                    IslandQueue.Add(Neighbor);
                    IslandSet.Add(Neighbor);
                    Visited.Add(Neighbor);
                }
            }
        }

        // Collect ring hexes: neighbors outside the island
        FHexCoordArray RingRegion;
        for (const FHexCoord& H : IslandSet)
        {
            for (const FHexCoord& N : GridWrapper.Grid.GetNeighbors(H))
            {
                if (!IslandSet.Contains(N))
                {
                    RingRegion.Hexes.Add(H);
                    break;
                }
            }
        }

        FHexCoordArray IslandRegion;
        IslandRegion.Hexes = IslandSet.Array();

        OutSelectedHexes.Add(IslandRegion);
        OutRingHexes.Add(RingRegion);
    }

    return OutSelectedHexes.Num() > 0;
}*/
#pragma endregion

