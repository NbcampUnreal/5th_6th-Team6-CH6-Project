// Fill out your copyright notice in the Description page of Project Settings.


#include "LineOfSight/WorldObstacle/LocalTextureSampler.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Canvas.h"
#include "Kismet/KismetRenderingLibrary.h"

#include "Engine/World.h"
#include "LineOfSight/VisionSubsystem.h"
#include "TopDownVisionLogCategories.h"//log
//#include "LineOfSight/GPU/TileMergeComputeShader.h"

ULocalTextureSampler::ULocalTextureSampler()
{
	PrimaryComponentTick.bCanEverTick = false;
}


// Called when the game starts
void ULocalTextureSampler::BeginPlay()
{
	Super::BeginPlay();

	PrepareSetups();
}


// Called every frame
void ULocalTextureSampler::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void ULocalTextureSampler::UpdateLocalTexture()
{
	if (!LocalMaskRT || !ObstacleSubsystem)
	{
		/*UE_LOG(LOSVision, Warning,
			TEXT("ULocalTextureSampler::UpdateLocalTexture >> Missing RT, MID, or Subsystem"));*/
		return;
	}

	const FVector WorldCenter = GetComponentLocation();
	LastSampleCenter = WorldCenter;

	/*UE_LOG(LOSVision, Log,
		TEXT("ULocalTextureSampler::UpdateLocalTexture >> WorldCenter: %s"),
		*WorldCenter.ToString());*/

	
	RebuildLocalBounds(WorldCenter);
	UpdateOverlappingTiles();
	DrawTilesIntoLocalRT();
}

void ULocalTextureSampler::SetWorldSampleRadius(float NewRadius)
{
	if (!FMath::IsNearlyEqual(WorldSampleRadius, NewRadius))
	{
		WorldSampleRadius = NewRadius;
		UE_LOG(LOSVision, Log,
			TEXT("ULocalTextureSampler::SetWorldSampleRadius >> New radius: %f"),
			WorldSampleRadius);
		
		UpdateLocalTexture();
	}
}

void ULocalTextureSampler::SetLocalRenderTarget(UTextureRenderTarget2D* InRT)
{
	if (LocalMaskRT == InRT)
	{
		UE_LOG(LOSVision, Warning,
			TEXT("ULocalTextureSampler::SetLocalRenderTarget >> Already using same RT"));
		return;
	}

	LocalMaskRT = InRT;//set

	if (!InRT)
	{
		UE_LOG(LOSVision, Warning,
			TEXT("ULocalTextureSampler::SetLocalRenderTarget >> RT is null"));
		return;
	}

	//force rebuild when RT is assigned
	UpdateLocalTexture();
}

void ULocalTextureSampler::PrepareSetups()
{
	// Grab the VisionSubsystem
	ObstacleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UVisionSubsystem>() : nullptr;
	if (!ObstacleSubsystem)
	{
		UE_LOG(LOSVision, Warning, TEXT("ULocalTextureSampler::Setup >> Failed to get VisionSubsystem"));
	}
}

void ULocalTextureSampler::RebuildLocalBounds(const FVector& WorldCenter)
{
	const FVector2D Center2D(WorldCenter.X, WorldCenter.Y);
	const float R = WorldSampleRadius;

	LocalWorldBounds = FBox2D(
		Center2D - FVector2D(R, R),
		Center2D + FVector2D(R, R)
	);
	
	/*
	UE_LOG(LOSVision, Log,
		TEXT("ULocalTextureSampler::RebuildLocalBounds >> Min: %s, Max: %s"),
		*LocalWorldBounds.Min.ToString(), *LocalWorldBounds.Max.ToString());*/
}

void ULocalTextureSampler::UpdateOverlappingTiles()
{
	ActiveTileIndices.Reset();

	const TArray<FObstacleMaskTile>& Tiles = ObstacleSubsystem->GetTiles();
	/*UE_LOG(LOSVision, Log,
		TEXT("ULocalTextureSampler::UpdateOverlappingTiles >> %d tiles in subsystem"),
		Tiles.Num());*/

	for (int32 i = 0; i < Tiles.Num(); ++i)
	{
		bool bOverlap = Tiles[i].WorldBounds.Intersect(LocalWorldBounds);
		
		/*UE_LOG(LOSVision, Log,
			TEXT("ULocalTextureSampler::UpdateOverlappingTiles >> Tile %d bounds [%s-%s], LocalBounds [%s-%s], Overlap=%d"),
			i,
			*Tiles[i].WorldBounds.Min.ToString(),
			*Tiles[i].WorldBounds.Max.ToString(),
			*LocalWorldBounds.Min.ToString(),
			*LocalWorldBounds.Max.ToString(),
			bOverlap
		);*/

		if (bOverlap)
		{
			ActiveTileIndices.Add(i);// add to the active tile array if overlapped
		}
	}

	/*UE_LOG(LOSVision, Log,
		TEXT("ULocalTextureSampler::UpdateOverlappingTiles >> %d tiles in local area"),
		ActiveTileIndices.Num());*/
}

void ULocalTextureSampler::DrawTilesIntoLocalRT()
{
    if (!LocalMaskRT || !ObstacleSubsystem)
    {
    	/*UE_LOG(LOSVision, Warning,
    		TEXT("ULocalTextureSampler::DrawTilesIntoLocalRT >> Missing RT, MID, or Subsystem"));*/
    	return;
    }

	UKismetRenderingLibrary::ClearRenderTarget2D(
		this,
		LocalMaskRT,
		FLinearColor::Black);

	if (ActiveTileIndices.IsEmpty())
		return;

	// Initialize Canvas tools
	FCanvas Canvas(
		LocalMaskRT->GameThread_GetRenderTargetResource(),
		nullptr,
		GetWorld(),
		GMaxRHIFeatureLevel);
    
	const FVector2D LocalSize = LocalWorldBounds.GetSize();

	for (int32 TileIndex : ActiveTileIndices)
	{
		const FObstacleMaskTile& Tile = ObstacleSubsystem->GetTiles()[TileIndex];
		if (!Tile.Mask) continue;

		// Calculate tile position relative to the RenderTarget (0.0 to 1.0 range or Pixel range)
		// You'll need to map Tile.WorldBounds to the LocalMaskRT's pixel space
		FVector2D TilePosInRT = (
			Tile.WorldBounds.Min - LocalWorldBounds.Min) / LocalSize * FVector2D(LocalMaskRT->SizeX,
			LocalMaskRT->SizeY);
		
		FVector2D TileSizeInRT = Tile.WorldBounds.GetSize() / LocalSize * FVector2D(LocalMaskRT->SizeX, LocalMaskRT->SizeY);

		// Create a Tile Item faster method, not requiring material
		FCanvasTileItem TileItem(TilePosInRT, Tile.Mask->GetResource(), TileSizeInRT, FLinearColor::White);
		TileItem.BlendMode = SE_BLEND_Additive; // Or AlphaBlend depending on your LOS logic
        
		// Queue the draw command
		Canvas.DrawItem(TileItem);
	}

	//  Tell the GPU to execute all queued draws at once!!!!
	Canvas.Flush_GameThread();
}