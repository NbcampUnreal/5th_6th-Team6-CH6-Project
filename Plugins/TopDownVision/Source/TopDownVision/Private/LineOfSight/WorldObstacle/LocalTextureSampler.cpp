// Fill out your copyright notice in the Description page of Project Settings.


#include "TopDownVision/Public/LineOfSight/WorldObstacle/LocalTextureSampler.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Canvas.h"
#include "Kismet/KismetRenderingLibrary.h"

#include "Engine/World.h"
#include "LineOfSight/VisionSubsystem.h"
#include "TopDownVisionLogCategories.h"//log
#include "LineOfSight/GPU/TileMergeComputeShader.h"

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
	if (!LocalMaskRT || !ProjectionMID || !ObstacleSubsystem)
	{
		UE_LOG(LOSVision, Warning,
			TEXT("ULocalTextureSampler::UpdateLocalTexture >> Missing RT, MID, or Subsystem"));
		return;
	}

	const FVector WorldCenter = GetComponentLocation();
	LastSampleCenter = WorldCenter;

	UE_LOG(LOSVision, Log,
		TEXT("ULocalTextureSampler::UpdateLocalTexture >> WorldCenter: %s"),
		*WorldCenter.ToString());

	
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

void ULocalTextureSampler::PrepareSetups()
{
	//MID
	if (TileProjectionMaterial)
	{
		ProjectionMID = UMaterialInstanceDynamic::Create(TileProjectionMaterial, this);
		if (!ProjectionMID)
		{
			UE_LOG(LOSVision, Error, TEXT("ULocalTextureSampler::Setup >> Failed to create MID"));
		}
	}
	else
	{
		UE_LOG(LOSVision, Warning, TEXT("ULocalTextureSampler::Setup >> BaseMaterial is null"));
	}

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
	
	UE_LOG(LOSVision, Log,
		TEXT("ULocalTextureSampler::RebuildLocalBounds >> Min: %s, Max: %s"),
		*LocalWorldBounds.Min.ToString(), *LocalWorldBounds.Max.ToString());
}

void ULocalTextureSampler::UpdateOverlappingTiles()
{
	ActiveTileIndices.Reset();

	const TArray<FObstacleMaskTile>& Tiles = ObstacleSubsystem->GetTiles();
	UE_LOG(LOSVision, Log,
		TEXT("ULocalTextureSampler::UpdateOverlappingTiles >> %d tiles in subsystem"),
		Tiles.Num());

	for (int32 i = 0; i < Tiles.Num(); ++i)
	{
		bool bOverlap = Tiles[i].WorldBounds.Intersect(LocalWorldBounds);
		
		UE_LOG(LOSVision, Log,
			TEXT("ULocalTextureSampler::UpdateOverlappingTiles >> Tile %d bounds [%s-%s], LocalBounds [%s-%s], Overlap=%d"),
			i,
			*Tiles[i].WorldBounds.Min.ToString(),
			*Tiles[i].WorldBounds.Max.ToString(),
			*LocalWorldBounds.Min.ToString(),
			*LocalWorldBounds.Max.ToString(),
			bOverlap
		);

		if (bOverlap)
		{
			ActiveTileIndices.Add(i);// add to the active tile array if overlapped
		}
	}

	UE_LOG(LOSVision, Log,
		TEXT("ULocalTextureSampler::UpdateOverlappingTiles >> %d tiles in local area"),
		ActiveTileIndices.Num());
}

void ULocalTextureSampler::DrawTilesIntoLocalRT()
{
    if (!LocalMaskRT || !ProjectionMID || !ObstacleSubsystem)
    {
    	UE_LOG(LOSVision, Warning,
    		TEXT("ULocalTextureSampler::DrawTilesIntoLocalRT >> Missing RT, MID, or Subsystem"));
    	return;
    }

	if (bUseCPU)
	{
		// Clear RT
		UKismetRenderingLibrary::ClearRenderTarget2D(
			this,
			LocalMaskRT,
			FLinearColor::Black
		);

		if (ActiveTileIndices.IsEmpty())
		{
			UE_LOG(LOSVision, Log,
				TEXT("ULocalTextureSampler::DrawTilesIntoLocalRT >> No active tiles to draw"));
			return;
		}

		const FVector2D LocalMin = LocalWorldBounds.Min;
		const FVector2D LocalSize = LocalWorldBounds.GetSize();

		for (int32 TileIndex : ActiveTileIndices)
		{
			const FObstacleMaskTile& Tile = ObstacleSubsystem->GetTiles()[TileIndex];
			if (!Tile.Mask)
				continue;

			// Calculate overlap
			FBox2D Overlap = Tile.WorldBounds;
			if (!Overlap.Intersect(LocalWorldBounds))
				continue;

			// Set material parameters
			ProjectionMID->SetTextureParameterValue(MIDParam_TextureObj, Tile.Mask);
        
			// Pass world bounds for shader calculation
			ProjectionMID->SetVectorParameterValue(MIDParam_TileWorldMin, 
				FVector(Tile.WorldBounds.Min, 0.f));
			ProjectionMID->SetVectorParameterValue(MIDParam_TileWorldMax, 
				FVector(Tile.WorldBounds.Max, 0.f));
			ProjectionMID->SetVectorParameterValue(MIDParam_LocalWorldMin, 
				FVector(LocalWorldBounds.Min, 0.f));
			ProjectionMID->SetVectorParameterValue(MIDParam_LocalWorldMax, 
				FVector(LocalWorldBounds.Max, 0.f));

			// FAST: Direct draw to render target
			UKismetRenderingLibrary::DrawMaterialToRenderTarget(
				this,
				LocalMaskRT,
				ProjectionMID
			);
		}
	
		UE_LOG(LOSVision, Log,
			TEXT("ULocalTextureSampler::DrawTilesIntoLocalRT >> Completed drawing tiles"));
		
		return;
	}
	else
	{
		// Prepare tile data
		TArray<FTileGPUData> TileData;
		TArray<FTexture2DRHIRef> TileTextureRHIs;
    
		for (int32 TileIndex : ActiveTileIndices)
		{
			const FObstacleMaskTile& Tile = ObstacleSubsystem->GetTiles()[TileIndex];
        
			if (!Tile.Mask || !Tile.Mask->GetResource())
				continue;

			FTileGPUData Data;
			Data.TileWorldMin = FVector2f(Tile.WorldBounds.Min);
			Data.TileWorldMax = FVector2f(Tile.WorldBounds.Max);
			Data.TextureIndex = TileData.Num();
			Data.Padding[0] = Data.Padding[1] = Data.Padding[2] = 0;
        
			TileData.Add(Data);
			TileTextureRHIs.Add(Tile.Mask->GetResource()->TextureRHI->GetTexture2D());
		}

		if (TileData.IsEmpty())
			return;

		// Get render target resource
		FTextureRenderTargetResource* RTResource = LocalMaskRT->GameThread_GetRenderTargetResource();
		if (!RTResource)
			return;

		// Dispatch to render thread
		const FVector2f LocalMin = FVector2f(LocalWorldBounds.Min);
		const FVector2f LocalMax = FVector2f(LocalWorldBounds.Max);
		const uint32 Resolution = LocalResolution;

		ENQUEUE_RENDER_COMMAND(MergeTilesCompute)(
			[
				RTResource,
				TileData,
				TileTextureRHIs,
				LocalMin,
				LocalMax,
				Resolution
				](FRHICommandListImmediate& RHICmdList)
			{
				TileMergeCS::Execute_RenderThread(
					RHICmdList,
					TileData,
					TileTextureRHIs,
					RTResource->GetRenderTargetTexture(),
					LocalMin,
					LocalMax,
					Resolution
				);
			}
		);
	}
  
}