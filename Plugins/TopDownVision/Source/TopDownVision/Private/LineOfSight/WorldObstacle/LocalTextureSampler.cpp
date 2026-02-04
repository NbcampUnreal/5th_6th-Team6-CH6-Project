// Fill out your copyright notice in the Description page of Project Settings.


#include "LineOfSight/WorldObstacle/LocalTextureSampler.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Canvas.h"
#include "Kismet/KismetRenderingLibrary.h"

#include "Engine/World.h"
#include "LineOfSight/VisionSubsystem.h"
#include "TopDownVisionLogCategories.h"//log

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
		UE_LOG(LOSVision, Verbose,
			TEXT("ULocalTextureSampler::UpdateLocalTexture >> Missing RT or Subsystem"));
		return;
	}

	const FVector WorldCenter = GetComponentLocation();
	LastSampleCenter = WorldCenter;

	UE_LOG(LOSVision, Verbose,
		TEXT("ULocalTextureSampler::UpdateLocalTexture >> WorldCenter: %s"),
		*WorldCenter.ToString());

	
	RebuildLocalBounds(WorldCenter);
	UpdateOverlappingTiles();
	DrawTilesIntoLocalRT();

	// Auto-update debug RT if enabled
	if (bAutoUpdateDebugRT && DebugRT)
	{
		UpdateDebugRT();
	}
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

void ULocalTextureSampler::UpdateDebugRT()
{
	if (!LocalMaskRT)
	{
		UE_LOG(LOSVision, Warning,
			TEXT("ULocalTextureSampler::UpdateDebugRT >> LocalMaskRT is null"));
		return;
	}

	if (!DebugRT)
	{
		UE_LOG(LOSVision, Warning,
			TEXT("ULocalTextureSampler::UpdateDebugRT >> DebugRT is null, please assign a debug render target"));
		return;
	}

	// Make sure DebugRT has the same size as LocalMaskRT
	if (DebugRT->SizeX != LocalMaskRT->SizeX || DebugRT->SizeY != LocalMaskRT->SizeY)
	{
		UE_LOG(LOSVision, Log,
			TEXT("ULocalTextureSampler::UpdateDebugRT >> Resizing DebugRT to match LocalMaskRT: %dx%d"),
			LocalMaskRT->SizeX, LocalMaskRT->SizeY);
		
		DebugRT->ResizeTarget(LocalMaskRT->SizeX, LocalMaskRT->SizeY);
	}

	// Copy LocalMaskRT to DebugRT
	FTextureRenderTargetResource* SrcResource = LocalMaskRT->GameThread_GetRenderTargetResource();
	FTextureRenderTargetResource* DstResource = DebugRT->GameThread_GetRenderTargetResource();

	if (!SrcResource || !DstResource)
	{
		UE_LOG(LOSVision, Error,
			TEXT("ULocalTextureSampler::UpdateDebugRT >> Failed to get render target resources"));
		return;
	}

	// Read pixels from source
	TArray<FColor> Pixels;
	SrcResource->ReadPixels(Pixels);

	// Write pixels to destination
	ENQUEUE_RENDER_COMMAND(CopyLocalMaskToDebug)(
		[DstResource, Pixels](FRHICommandListImmediate& RHICmdList)
		{
			FUpdateTextureRegion2D Region(
				0, 0, 0, 0,
				DstResource->GetSizeX(),
				DstResource->GetSizeY()
			);

			RHIUpdateTexture2D(
				DstResource->GetRenderTargetTexture(),
				0,
				Region,
				DstResource->GetSizeX() * 4,
				reinterpret_cast<const uint8*>(Pixels.GetData())
			);
		}
	);

	UE_LOG(LOSVision, Verbose,
		TEXT("ULocalTextureSampler::UpdateDebugRT >> Updated debug RT with %d pixels"),
		Pixels.Num());
}

void ULocalTextureSampler::PrepareSetups()
{
	// Grab the VisionSubsystem
	ObstacleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UVisionSubsystem>() : nullptr;
	if (!ObstacleSubsystem)
	{
		UE_LOG(LOSVision, Warning, TEXT("ULocalTextureSampler::PrepareSetups >> Failed to get VisionSubsystem"));
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
	
	
	UE_LOG(LOSVision, VeryVerbose,
		TEXT("ULocalTextureSampler::RebuildLocalBounds >> Min: %s, Max: %s"),
		*LocalWorldBounds.Min.ToString(), *LocalWorldBounds.Max.ToString());
}

void ULocalTextureSampler::UpdateOverlappingTiles()
{
	ActiveTileIndices.Reset();

	const TArray<FObstacleMaskTile>& Tiles = ObstacleSubsystem->GetTiles();
	UE_LOG(LOSVision, Verbose,
		TEXT("ULocalTextureSampler::UpdateOverlappingTiles >> %d tiles in subsystem"),
		Tiles.Num());

	for (int32 i = 0; i < Tiles.Num(); ++i)
	{
		bool bOverlap = Tiles[i].WorldBounds.Intersect(LocalWorldBounds);
		
		UE_LOG(LOSVision, VeryVerbose,
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

	UE_LOG(LOSVision, Verbose,
		TEXT("ULocalTextureSampler::UpdateOverlappingTiles >> %d tiles in local area"),
		ActiveTileIndices.Num());
}

void ULocalTextureSampler::DrawTilesIntoLocalRT()
{
    if (!LocalMaskRT || !ObstacleSubsystem)
    {
    	UE_LOG(LOSVision, Verbose,
    		TEXT("ULocalTextureSampler::DrawTilesIntoLocalRT >> Missing RT or Subsystem"));
    	return;
    }

	UKismetRenderingLibrary::ClearRenderTarget2D(
		this,
		LocalMaskRT,
		FLinearColor::Black);

	if (ActiveTileIndices.IsEmpty())
	{
		UE_LOG(LOSVision, Verbose,
			TEXT("ULocalTextureSampler::DrawTilesIntoLocalRT >> No active tiles"));
		return;
	}

	// Initialize Canvas tools
	FCanvas Canvas(
		LocalMaskRT->GameThread_GetRenderTargetResource(),
		nullptr,
		GetWorld(),
		GMaxRHIFeatureLevel);
    
	const FVector2D LocalSize = LocalWorldBounds.GetSize();
	
	// Scene capture camera rotation constant
	// Camera is at Rotation(-90, 90, 0) which creates a 90-degree transformation
	const float CameraYawOffset = 90.f;

	UE_LOG(LOSVision, Verbose,
		TEXT("ULocalTextureSampler::DrawTilesIntoLocalRT >> Drawing %d tiles"),
		ActiveTileIndices.Num());

	for (int32 TileIndex : ActiveTileIndices)
	{
		const FObstacleMaskTile& Tile = ObstacleSubsystem->GetTiles()[TileIndex];
		if (!Tile.Mask)
		{
			UE_LOG(LOSVision, VeryVerbose,
				TEXT("ULocalTextureSampler::DrawTilesIntoLocalRT >> Tile %d has no mask, skipping"),
				TileIndex);
			continue;
		}

		// World position of tile center
		FVector2D TileCenterWorld = Tile.WorldCenter;
		
		// Transform to local space (relative to LocalWorldBounds)
		FVector2D LocalRelative = TileCenterWorld - LocalWorldBounds.Min;
		
		// Apply camera rotation transformation
		// Camera Yaw=90 means: World +X maps to RT +Y, World +Y maps to RT -X
		FVector2D RotatedLocal;
		RotatedLocal.X = LocalRelative.Y;  // World Y becomes RT X
		RotatedLocal.Y = LocalSize.X - LocalRelative.X;  // World X becomes RT Y (flipped)
		
		// Convert to RT pixel coordinates
		FVector2D TileCenterInRT;
		TileCenterInRT.X = (RotatedLocal.X / LocalSize.Y) * LocalMaskRT->SizeX;
		TileCenterInRT.Y = (RotatedLocal.Y / LocalSize.X) * LocalMaskRT->SizeY;
		
		// Calculate tile size (swap X/Y due to 90-degree rotation)
		FVector2D TileSizeInRT;
		TileSizeInRT.X = (Tile.WorldSize.Y / LocalSize.Y) * LocalMaskRT->SizeX;
		TileSizeInRT.Y = (Tile.WorldSize.X / LocalSize.X) * LocalMaskRT->SizeY;

		// Calculate top-left position (Canvas draws from top-left)
		FVector2D TilePosInRT = TileCenterInRT - (TileSizeInRT * 0.5f);

		UE_LOG(LOSVision, VeryVerbose,
			TEXT("ULocalTextureSampler::DrawTilesIntoLocalRT >> "
		"Tile %d: WorldCenter=(%.1f, %.1f), RTCenter=(%.1f, %.1f), Pos=(%.1f, %.1f), Size=(%.1f, %.1f), TileRot=%.1f, FinalRot=%.1f"),
			TileIndex,
			TileCenterWorld.X, TileCenterWorld.Y,
			TileCenterInRT.X, TileCenterInRT.Y,
			TilePosInRT.X, TilePosInRT.Y,
			TileSizeInRT.X, TileSizeInRT.Y,
			Tile.WorldRotationYaw,
			Tile.WorldRotationYaw - CameraYawOffset);

		// Create tile item
		FCanvasTileItem TileItem(TilePosInRT, Tile.Mask->GetResource(), TileSizeInRT, FLinearColor::White);
		TileItem.BlendMode = SE_BLEND_Additive;
		
		// Apply rotation: compensate for camera yaw + apply tile's world rotation
		TileItem.PivotPoint = FVector2D(0.5f, 0.5f);
		TileItem.Rotation = FRotator(0.f, Tile.WorldRotationYaw - CameraYawOffset, 0.f);
		
		Canvas.DrawItem(TileItem);
	}

	//  Tell the GPU to execute all queued draws at once!!!!
	Canvas.Flush_GameThread();

	UE_LOG(LOSVision, Verbose,
		TEXT("ULocalTextureSampler::DrawTilesIntoLocalRT >> Finished drawing tiles"));
}
