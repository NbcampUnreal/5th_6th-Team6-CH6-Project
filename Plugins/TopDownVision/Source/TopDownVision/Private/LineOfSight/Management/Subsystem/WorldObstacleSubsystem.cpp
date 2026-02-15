// Fill out your copyright notice in the Description page of Project Settings.


#include "LineOfSight/Management/Subsystem/WorldObstacleSubsystem.h"
#include "LineOfSight/LineOfSightComponent.h"

//LOG
DEFINE_LOG_CATEGORY(ObstacleSubsystem);

void UWorldObstacleSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	UE_LOG(ObstacleSubsystem, Log,
		TEXT("UWorldObstacleSubsystem::Initialize >> Vision Subsystem Initialized"));

	//load from level data asset hub
	LoadAndInitializeTiles();
}

void UWorldObstacleSubsystem::Deinitialize()
{
	UE_LOG(ObstacleSubsystem, Log,
		TEXT("UWorldObstacleSubsystem::Deinitialize >> VisionSubsystem Deinitialize"));
	
	Super::Deinitialize();
}

bool UWorldObstacleSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	if (World)
	{
		// Create in game world AND editor world
		return true;
	}
	return false;
}


void UWorldObstacleSubsystem::RequestObstacleBake(EObstacleBakeRequest Request)
{
	if (OnObstacleBakeRequested.IsBound())
	{
		OnObstacleBakeRequested.Broadcast(Request);
		UE_LOG(ObstacleSubsystem, Log,
			TEXT(" UWorldObstacleSubsystem::RequestObstacleBake >> Broadcasted request %d"),
			static_cast<int32>(Request));
	}
	else
	{
		UE_LOG(ObstacleSubsystem, Warning,
			TEXT(" UWorldObstacleSubsystem::RequestObstacleBake >> No listeners bound to delegate"));
	}
}

void UWorldObstacleSubsystem::RegisterObstacleTile(FObstacleMaskTile NewTile)
{
	if (!NewTile.Mask)
	{
		UE_LOG(ObstacleSubsystem, Warning,
			TEXT("UWorldObstacleSubsystem::RegisterObstacleTile >> "
		"Trying to register a tile without a valid mask"));
		return;
	}

	// Add tile to array
	WorldTiles.Add(NewTile);

	UE_LOG(ObstacleSubsystem, Log,
		TEXT("UWorldObstacleSubsystem::RegisterObstacleTile >> "
	   "Registered tile at bounds Min=(%.1f, %.1f) Max=(%.1f, %.1f)"),
		NewTile.WorldBounds.Min.X,
		NewTile.WorldBounds.Min.Y,
		NewTile.WorldBounds.Max.X,
		NewTile.WorldBounds.Max.Y);
}

void UWorldObstacleSubsystem::ClearObstacleTiles()
{
	const int32 NumTiles = WorldTiles.Num();
	WorldTiles.Reset();

	UE_LOG(ObstacleSubsystem, Log,
		TEXT("UWorldObstacleSubsystem::ClearObstacleTiles >> Cleared %d obstacle tiles"), NumTiles);
}

void UWorldObstacleSubsystem::InitializeTilesFromDataAsset(ULevelObstacleData* TileDataForWorld)
{
	if (!TileDataForWorld)
	{
		UE_LOG(ObstacleSubsystem, Warning,
			TEXT("UWorldObstacleSubsystem::InitializeTilesFromDataAsset >> No TileData provided"));
		return;
	}

	WorldTiles.Reset();

	for (const FObstacleMaskTile& Tile : TileDataForWorld->Tiles)
	{
		WorldTiles.Add(Tile);

		UE_LOG(ObstacleSubsystem, Log,
			TEXT("UWorldObstacleSubsystem::InitializeTilesFromDataAsset >> Loaded tile at bounds Min=(%.1f, %.1f) Max=(%.1f, %.1f)"),
			Tile.WorldBounds.Min.X, Tile.WorldBounds.Min.Y,
			Tile.WorldBounds.Max.X, Tile.WorldBounds.Max.Y);
	}
}

void UWorldObstacleSubsystem::RemoveTileByTexture(UTexture2D* Texture)
{
	if (!Texture) return;

	for (int32 i = WorldTiles.Num() - 1; i >= 0; --i)
	{
		if (WorldTiles[i].Mask == Texture)
		{
			WorldTiles.RemoveAt(i);
		}
	}
}

void UWorldObstacleSubsystem::GetOverlappingTileIndices(const FBox2D& QueryBounds, TArray<int32>& OutIndices) const
{
	OutIndices.Reset();

	if (!QueryBounds.bIsValid)
	{
		return;
	}

	for (int32 i = 0; i < WorldTiles.Num(); ++i)
	{
		if (WorldTiles[i].WorldBounds.Intersect(QueryBounds))
		{
			OutIndices.Add(i);
		}
	}
}

void UWorldObstacleSubsystem::LoadAndInitializeTiles()
{
	UE_LOG(ObstacleSubsystem, Warning,
        TEXT("UWorldObstacleSubsystem::LoadAndInitializeTiles >> Starting tile load..."));

    if (!LevelObstacleDataPath.IsValid())
    {
        UE_LOG(ObstacleSubsystem, Error,
            TEXT("UWorldObstacleSubsystem::LoadAndInitializeTiles >> LevelObstacleDataPath is INVALID! Path: %s"),
            *LevelObstacleDataPath.ToString());
        return;
    }

    UE_LOG(ObstacleSubsystem, Warning,
        TEXT("UWorldObstacleSubsystem::LoadAndInitializeTiles >> Attempting to load from path: %s"),
        *LevelObstacleDataPath.ToString());

    UObject* LoadedObj = LevelObstacleDataPath.TryLoad();
    
    if (!LoadedObj)
    {
        UE_LOG(ObstacleSubsystem, Error,
            TEXT("UWorldObstacleSubsystem::LoadAndInitializeTiles >> TryLoad() FAILED! Asset not found at: %s"),
            *LevelObstacleDataPath.ToString());
        return;
    }

    UE_LOG(ObstacleSubsystem, Warning,
        TEXT("UWorldObstacleSubsystem::LoadAndInitializeTiles >> Successfully loaded object: %s (Class: %s)"),
        *LoadedObj->GetName(),
        *LoadedObj->GetClass()->GetName());

    UWorldRequirementList* WorldReqList = Cast<UWorldRequirementList>(LoadedObj);
    
    if (!WorldReqList)
    {
        UE_LOG(ObstacleSubsystem, Error,
            TEXT("UWorldObstacleSubsystem::LoadAndInitializeTiles >> Cast to UWorldRequirementList FAILED! "
                 "Loaded object class: %s"),
            *LoadedObj->GetClass()->GetName());
        return;
    }

    UE_LOG(ObstacleSubsystem, Warning,
        TEXT("UWorldObstacleSubsystem::LoadAndInitializeTiles >> Cast successful, WorldReqList has %d entries"),
        WorldReqList->WorldRequirements.Num());

    // Debug: Print all available world names in the map
    for (const auto& Pair : WorldReqList->WorldRequirements)
    {
        UE_LOG(ObstacleSubsystem, Warning,
            TEXT("UWorldObstacleSubsystem::LoadAndInitializeTiles >> Available world key: '%s'"),
            *Pair.Key);
    }

    if (!GetWorld())
    {
        UE_LOG(ObstacleSubsystem, Error,
            TEXT("UWorldObstacleSubsystem::LoadAndInitializeTiles >> GetWorld() returned NULL!"));
        return;
    }
    
    FString MapPackageLongName = GetWorld()->GetMapName();
    
    UE_LOG(ObstacleSubsystem, Warning,
        TEXT("UWorldObstacleSubsystem::LoadAndInitializeTiles >> Current world name (before prefix removal): '%s'"),
        *MapPackageLongName);
    
    UE_LOG(ObstacleSubsystem, Warning,
        TEXT("UWorldObstacleSubsystem::LoadAndInitializeTiles >> Streaming levels prefix: '%s'"),
        *GetWorld()->StreamingLevelsPrefix);

    MapPackageLongName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);

    UE_LOG(ObstacleSubsystem, Warning,
        TEXT("UWorldObstacleSubsystem::LoadAndInitializeTiles >> Current world name (after prefix removal): '%s'"),
        *MapPackageLongName);

    if (TObjectPtr<ULevelObstacleData>* TileDataPtr = WorldReqList->WorldRequirements.Find(MapPackageLongName))
    {
        if (*TileDataPtr)
        {
            UE_LOG(ObstacleSubsystem, Warning,
                TEXT("UWorldObstacleSubsystem::LoadAndInitializeTiles >> FOUND tile data for world: '%s' with %d tiles"),
                *MapPackageLongName,
                (*TileDataPtr)->Tiles.Num());

            InitializeTilesFromDataAsset(*TileDataPtr);
        }
        else
        {
            UE_LOG(ObstacleSubsystem, Error,
                TEXT("UWorldObstacleSubsystem::LoadAndInitializeTiles >> TileData pointer is NULL for world: '%s'"),
                *MapPackageLongName);
        }
    }
    else
    {
        UE_LOG(ObstacleSubsystem, Error,
            TEXT("UWorldObstacleSubsystem::LoadAndInitializeTiles >> NO MATCH FOUND for world name: '%s'"),
            *MapPackageLongName);
        
        UE_LOG(ObstacleSubsystem, Error,
            TEXT("UWorldObstacleSubsystem::LoadAndInitializeTiles >> Available keys in WorldRequirements:"));
        
        for (const auto& Pair : WorldReqList->WorldRequirements)
        {
            UE_LOG(ObstacleSubsystem, Error,
                TEXT("  - '%s' (matches: %s)"),
                *Pair.Key,
                (Pair.Key == MapPackageLongName) ? TEXT("YES") : TEXT("NO"));
        }
    }
}

