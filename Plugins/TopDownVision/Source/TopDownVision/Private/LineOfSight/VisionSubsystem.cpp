// Fill out your copyright notice in the Description page of Project Settings.


#include "LineOfSight/VisionSubsystem.h"
#include "LineOfSight/LineOfSightComponent.h"

//LOG
DEFINE_LOG_CATEGORY(VisionSubsystem);

void UVisionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	UE_LOG(VisionSubsystem, Log, TEXT("UVisionSubsystem::Initialize >> Vision Subsystem Initialized"));

	//load from level data asset hub
	LoadAndInitializeTiles();
}

void UVisionSubsystem::Deinitialize()
{
	UE_LOG(VisionSubsystem, Log,
		TEXT("UVisionSubsystem::Deinitialize >> VisionSubsystem Deinitialize"));
	
	Super::Deinitialize();
}

bool UVisionSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	if (World)
	{
		// Create in game world AND editor world
		return true;
	}
	return false;
}

bool UVisionSubsystem::RegisterProvider(ULineOfSightComponent* Provider, EVisionChannel InVisionChannel)
{
	if (!Provider)
	{
		UE_LOG(VisionSubsystem, Error,
			TEXT("UVisionSubsystem::RegisterProvider >>Invalid provider"));
		return false;
	}
	if (InVisionChannel==EVisionChannel::None)
	{
		UE_LOG(VisionSubsystem, Error,
			TEXT("UVisionSubsystem::RegisterProvider >>VisionChannel not settled"));
		return false;
	}

	// Get or create the channel entry
	FRegisteredProviders& ChannelEntry = VisionMap.FindOrAdd(InVisionChannel);
	if (ChannelEntry.RegisteredList.Contains(Provider))
	{
		UE_LOG(VisionSubsystem, Warning,
			TEXT("UVisionSubsystem::RegisterProvider >> Already registered provider[%s] in channel:%d"),
			*Provider->GetOwner()->GetName(),
			InVisionChannel);
		return false;
	}

	// Add to the list
	ChannelEntry.RegisteredList.Add(Provider);
	UE_LOG(VisionSubsystem, Log,
		TEXT("UVisionSubsystem::RegisterProvider >> Provider[%s] registered. channel:%d"),
		*Provider->GetOwner()->GetName(),
		InVisionChannel);

	return true;
}

void UVisionSubsystem::UnregisterProvider(ULineOfSightComponent* Provider, EVisionChannel InVisionChannel)
{
	if (!Provider)
	{
		UE_LOG(VisionSubsystem, Error, TEXT("UVisionSubsystem::UnregisterProvider >> Invalid provider"));
		return;
	}

	// Try to find channel
	if (FRegisteredProviders* ChannelEntry = VisionMap.Find(InVisionChannel))
	{
		// Remove provider if it exists
		if (ChannelEntry->RegisteredList.Remove(Provider) > 0)
		{
			UE_LOG(VisionSubsystem, Log,
				TEXT("UVisionSubsystem::UnregisterProvider >> Provider[%s] unregistered from channel:%d"),
				*Provider->GetOwner()->GetName(),
				InVisionChannel);
			return; // Success, stop
		}
	}

	// Provider not found in channel
	UE_LOG(VisionSubsystem, Warning,
		TEXT("UVisionSubsystem::UnregisterProvider >> Could not find Provider[%s] in channel:%d"),
		*Provider->GetOwner()->GetName(),
		InVisionChannel);
}

void UVisionSubsystem::RequestObstacleBake(EObstacleBakeRequest Request)
{
	if (OnObstacleBakeRequested.IsBound())
	{
		OnObstacleBakeRequested.Broadcast(Request);
		UE_LOG(VisionSubsystem, Log,
			TEXT(" UVisionSubsystem::RequestObstacleBake >> Broadcasted request %d"),
			static_cast<int32>(Request));
	}
	else
	{
		UE_LOG(VisionSubsystem, Warning,
			TEXT(" UVisionSubsystem::RequestObstacleBake >> No listeners bound to delegate"));
	}
}

void UVisionSubsystem::RegisterObstacleTile(FObstacleMaskTile NewTile)
{
	if (!NewTile.Mask)
	{
		UE_LOG(VisionSubsystem, Warning,
			TEXT("UVisionSubsystem::RegisterObstacleTile >> "
		"Trying to register a tile without a valid mask"));
		return;
	}

	// Add tile to array
	WorldTiles.Add(NewTile);

	UE_LOG(VisionSubsystem, Log,
		TEXT("UVisionSubsystem::RegisterObstacleTile >> "
	   "Registered tile at bounds Min=(%.1f, %.1f) Max=(%.1f, %.1f)"),
		NewTile.WorldBounds.Min.X,
		NewTile.WorldBounds.Min.Y,
		NewTile.WorldBounds.Max.X,
		NewTile.WorldBounds.Max.Y);
}

void UVisionSubsystem::ClearObstacleTiles()
{
	const int32 NumTiles = WorldTiles.Num();
	WorldTiles.Reset();

	UE_LOG(VisionSubsystem, Log,
		TEXT("UVisionSubsystem::ClearObstacleTiles >> Cleared %d obstacle tiles"), NumTiles);
}

void UVisionSubsystem::InitializeTilesFromDataAsset(ULevelObstacleData* TileDataForWorld)
{
	if (!TileDataForWorld)
	{
		UE_LOG(VisionSubsystem, Warning,
			TEXT("UVisionSubsystem::InitializeTilesFromDataAsset >> No TileData provided"));
		return;
	}

	WorldTiles.Reset();

	for (const FObstacleMaskTile& Tile : TileDataForWorld->Tiles)
	{
		WorldTiles.Add(Tile);

		UE_LOG(VisionSubsystem, Log,
			TEXT("UVisionSubsystem::InitializeTilesFromDataAsset >> Loaded tile at bounds Min=(%.1f, %.1f) Max=(%.1f, %.1f)"),
			Tile.WorldBounds.Min.X, Tile.WorldBounds.Min.Y,
			Tile.WorldBounds.Max.X, Tile.WorldBounds.Max.Y);
	}
}

void UVisionSubsystem::RemoveTileByTexture(UTexture2D* Texture)
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


void UVisionSubsystem::GetOverlappingTileIndices(const FBox2D& QueryBounds, TArray<int32>& OutIndices) const
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

TArray<ULineOfSightComponent*> UVisionSubsystem::GetProvidersForTeam(EVisionChannel TeamChannel) const
{
	TArray<ULineOfSightComponent*> OutProviders;

	// Add providers from the requested team channel
	if (const FRegisteredProviders* TeamEntry = VisionMap.Find(TeamChannel))
	{
		OutProviders.Append(TeamEntry->RegisteredList);
	}
	else
	{
		UE_LOG(VisionSubsystem, Error,
			TEXT("UVisionSubsystem::GetProvidersForTeam >> No providers found for team channel:%d"),
			TeamChannel);
	}

	// Add providers from shared vision channel
	if (const FRegisteredProviders* SharedEntry = VisionMap.Find(EVisionChannel::SharedVision))
	{
		OutProviders.Append(SharedEntry->RegisteredList);
	}
	else
	{
		UE_LOG(VisionSubsystem, Error,
			TEXT("UVisionSubsystem::GetProvidersForTeam >> No shared vision providers found"));
	}

	return OutProviders;
}



void UVisionSubsystem::LoadAndInitializeTiles()
{
	if (!LevelObstacleDataPath.IsValid())
	{
		UE_LOG(VisionSubsystem, Warning,
			TEXT("UVisionSubsystem::LoadAndInitializeTiles >> LevelObstacleDataPath is invalid"));
		return;
	}

	UObject* LoadedObj = LevelObstacleDataPath.TryLoad();
	if (!LoadedObj)
	{
		UE_LOG(VisionSubsystem, Warning,
			TEXT("UVisionSubsystem::LoadAndInitializeTiles >> Failed to load asset at path: %s"),
			*LevelObstacleDataPath.ToString());
		return;
	}

	UWorldRequirementList* WorldReqList = Cast<UWorldRequirementList>(LoadedObj);
	if (!WorldReqList)
	{
		UE_LOG(VisionSubsystem, Warning,
			TEXT("UVisionSubsystem::LoadAndInitializeTiles >> Loaded object is not a UWorldRequirementList"));
		return;
	}

	if (!GetWorld())
	{
		UE_LOG(VisionSubsystem, Warning,
			TEXT("UVisionSubsystem::LoadAndInitializeTiles >> GetWorld() returned null"));
		return;
	}

	// built-in function for shortening the name
	FString MapPackageLongName = GetWorld()->GetMapName();
	MapPackageLongName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);//get the prevbi
	//FString MapPackageShortName = FPackageName::GetShortName(MapPackageLongName);

	if (TObjectPtr<ULevelObstacleData>* TileDataPtr = WorldReqList->WorldRequirements.Find(MapPackageLongName))
	{
		if (*TileDataPtr)
		{
			UE_LOG(VisionSubsystem, Log,
				TEXT("UVisionSubsystem::LoadAndInitializeTiles >> Initializing tiles for world: %s"),
				*MapPackageLongName);

			InitializeTilesFromDataAsset(*TileDataPtr);
		}
		else
		{
			UE_LOG(VisionSubsystem, Warning,
				TEXT("UVisionSubsystem::LoadAndInitializeTiles >> TileData is null for world: %s"),
				*MapPackageLongName);
		}
	}
	else
	{
		UE_LOG(VisionSubsystem, Warning,
			TEXT("UVisionSubsystem::LoadAndInitializeTiles >> No tile data found for world: %s"),
			*MapPackageLongName);
	}
}
