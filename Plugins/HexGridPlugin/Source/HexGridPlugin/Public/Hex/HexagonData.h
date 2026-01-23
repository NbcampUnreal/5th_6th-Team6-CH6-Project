#pragma once

#include "CoreMinimal.h"
#include "CPU/Interface/GridNodeInterface.h"
#include "HexagonData.generated.h"

//========================== HexCoord ================================================================================//
#pragma region HexCoord
/**
 * Cube-coordinate hex position
 * Constraint: X + Y + Z = 0
 */
USTRUCT(BlueprintType)
struct FHexCoord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Hex_X;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Hex_Y;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Hex_Z;

	// ---------- Constructors ----------

	FHexCoord() : Hex_X(0), Hex_Y(0), Hex_Z(0) {} // default constructor

	FHexCoord(int32 InX, int32 InY, int32 InZ) : Hex_X(InX), Hex_Y(InY), Hex_Z(InZ)
	{
		check(Hex_X + Hex_Y + Hex_Z == 0);
	}

	// Axial (q, r) → Cube
	static FHexCoord FromAxial(int32 Q, int32 R)
	{
		return FHexCoord(Q, -Q - R, R);
	}

	// Cube → Axial
	FIntPoint ToAxial() const
	{
		return FIntPoint(Hex_X, Hex_Z);
	}

	// ---------- Operators ----------

	bool operator==(const FHexCoord& Other) const
	{
		return Hex_X == Other.Hex_X && Hex_Y == Other.Hex_Y && Hex_Z == Other.Hex_Z;
	}

	bool operator!=(const FHexCoord& Other) const
	{
		return !(*this == Other);
	}

	FHexCoord operator+(const FHexCoord& Other) const
	{
		return FHexCoord(Hex_X + Other.Hex_X, Hex_Y + Other.Hex_Y, Hex_Z + Other.Hex_Z);
	}

	FHexCoord operator-(const FHexCoord& Other) const
	{
		return FHexCoord(Hex_X - Other.Hex_X, Hex_Y - Other.Hex_Y, Hex_Z - Other.Hex_Z);
	}

	FHexCoord operator*(int32 Scalar) const
	{
		return FHexCoord(Hex_X * Scalar, Hex_Y * Scalar, Hex_Z * Scalar);
	}

	// ---------- Directions ----------

	static const FHexCoord Directions[6];

	FHexCoord Neighbor(int32 Direction) const
	{
		check(Direction >= 0 && Direction < 6);
		return *this + Directions[Direction];
	}

	// Optional typed version
	enum class EHexDirection : uint8
	{
		NE = 0,
		E  = 1,
		SE = 2,
		SW = 3,
		W  = 4,
		NW = 5
	};

	FHexCoord Neighbor(EHexDirection Dir) const
	{
		return Neighbor(static_cast<int32>(Dir));
	}

	// ---------- Distance ----------

	int32 DistanceTo(const FHexCoord& Other) const
	{
		return (FMath::Abs(Hex_X - Other.Hex_X)
			  + FMath::Abs(Hex_Y - Other.Hex_Y)
			  + FMath::Abs(Hex_Z - Other.Hex_Z)) / 2;
	}

	// ---------- Hash ----------

	friend uint32 GetTypeHash(const FHexCoord& Hex)
	{
		return HashCombine(
			HashCombine(::GetTypeHash(Hex.Hex_X), ::GetTypeHash(Hex.Hex_Y)),
			::GetTypeHash(Hex.Hex_Z)
		);
	}

	// ---------- Validation ----------

	bool IsValid() const
	{
		return (Hex_X + Hex_Y + Hex_Z) == 0;
	}
};
#pragma endregion
//--------------------------------------------------------------------------------------------------------------------//

//===================== HexGridLayout ================================================================================//
#pragma region HexGridLayout

UENUM(BlueprintType)
enum class EHexOrientation : uint8
{
	PointyTop,
	FlatTop
};

USTRUCT(BlueprintType)
struct FHexGridLayout
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EHexOrientation Orientation = EHexOrientation::PointyTop;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float HexSize = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D Origin = FVector2D::ZeroVector;

	// coord → World location(2D)
	FVector2D HexToWorld(const FHexCoord& Hex) const
	{
		if (Orientation == EHexOrientation::PointyTop)
		{
			float X = HexSize * (FMath::Sqrt(3.f) * Hex.Hex_X + FMath::Sqrt(3.f) / 2.f * Hex.Hex_Z);
			float Y = HexSize * (3.f / 2.f * Hex.Hex_Z);
			return Origin + FVector2D(X, Y);
		}
		else // FlatTop
		{
			float X = HexSize * (3.f / 2.f * Hex.Hex_X);
			float Y = HexSize * (FMath::Sqrt(3.f) * Hex.Hex_Z + FMath::Sqrt(3.f) / 2.f * Hex.Hex_X);
			return Origin + FVector2D(X, Y);
		}
	}
	// reverse World to Coord
	FHexCoord WorldToHex(const FVector2D& WorldPos) const
	{
		// Convert world coordinates to hex axial
		const float q = (sqrt(3.f)/3.f * (WorldPos.X - Origin.X) - 1.f/3.f * (WorldPos.Y - Origin.Y)) / HexSize;
		const float r = (2.f/3.f * (WorldPos.Y - Origin.Y)) / HexSize;

		// Convert axial to cube coordinates
		int32 x = FMath::RoundToInt(q);
		int32 z = FMath::RoundToInt(r);
		int32 y = -x - z;

		return FHexCoord(x, y, z);
	}
};

#pragma endregion
//--------------------------------------------------------------------------------------------------------------------//

//===================== HexGrid Container ============================================================================//
#pragma region HexGrid Container

USTRUCT(BlueprintType)
struct FHexGrid
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TSet<FHexCoord> Cells;

	// ---------- Generation ----------

	void GenerateHexagon(int32 Radius)
	{
		Cells.Empty();

		for (int32 x = -Radius; x <= Radius; x++)
		{
			for (int32 y = FMath::Max(-Radius, -x - Radius);
					 y <= FMath::Min(Radius, -x + Radius);
					 y++)
			{
				int32 z = -x - y;
				Cells.Add(FHexCoord(x, y, z));
			}
		}
	}

	// ---------- Queries ----------

	bool Contains(const FHexCoord& Hex) const
	{
		return Cells.Contains(Hex);
	}

	TArray<FHexCoord> GetNeighbors(const FHexCoord& Hex) const
	{
		TArray<FHexCoord> Result;

		for (int32 i = 0; i < 6; i++)
		{
			FHexCoord N = Hex.Neighbor(i);
			if (Cells.Contains(N))
			{
				Result.Add(N);
			}
		}
		return Result;
	}

	TArray<FHexCoord> GetRing(const FHexCoord& Center, int32 Radius) const
	{
		TArray<FHexCoord> Result;

		if (Radius == 0)
		{
			Result.Add(Center);
			return Result;
		}

		FHexCoord Cube = Center + FHexCoord::Directions[4] * Radius;

		for (int32 i = 0; i < 6; i++)
		{
			for (int32 j = 0; j < Radius; j++)
			{
				if (Cells.Contains(Cube))
				{
					Result.Add(Cube);
				}
				Cube = Cube.Neighbor(i);
			}
		}
		return Result;
	}
};
#pragma endregion
//--------------------------------------------------------------------------------------------------------------------//
//======= Wrapper Data (FHexGrid + FHexGridLayout + GridCenter +  WorldPositions) ====================================//

#pragma region Wrapper Struct

USTRUCT(BlueprintType)
struct FHexGridWrapper
{
	GENERATED_BODY()
	
	/** The set of all hex coordinates in the grid */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex Grid")
	FHexGrid Grid;

	/** Layout info (size, origin, orientation) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hex Grid")
	FHexGridLayout Layout;

	/** Optional cached world positions for fast access */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex Grid")
	TMap<FHexCoord, FVector2D> WorldPositions;

	/** Optional grid origin in world space (could be the actor location) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hex Grid")
	FVector GridCenter = FVector::ZeroVector;
	
	//actual cell size (distance from center to vertex)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex Grid")
	float CellSize = 100.f;
	
	/** Default constructor */
	FHexGridWrapper() {}

	/** Convenience constructor */
	FHexGridWrapper(const FHexGrid& InGrid, const FHexGridLayout& InLayout, const FVector& InCenter)
		: Grid(InGrid), Layout(InLayout), GridCenter(InCenter)
	{
		CacheWorldPositions();
	}

	/** Fill the WorldPositions map for all hexes */
	void CacheWorldPositions()
	{
		WorldPositions.Empty();
		for (const FHexCoord& Hex : Grid.Cells)
		{
			FVector2D Pos2D = Layout.HexToWorld(Hex);
			WorldPositions.Add(Hex, Pos2D);
		}
	}

	/** Get world position for a given hex */
	FVector2D GetWorldPos(const FHexCoord& Hex) const
	{
		const FVector2D* Pos = WorldPositions.Find(Hex);
		if (Pos)
			return *Pos;

		return Layout.HexToWorld(Hex); // fallback
	}

	/** Clear all hexes */
	void Clear()
	{
		Grid.Cells.Empty();
		WorldPositions.Empty();
	}
};
#pragma endregion