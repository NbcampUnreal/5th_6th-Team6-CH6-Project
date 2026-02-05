// Fill out your copyright notice in the Description page of Project Settings.

#include "LineOfSight/WorldObstacle/LocalTextureSampler.h"

#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/World.h"
#include "LineOfSight/VisionSubsystem.h"
#include "TopDownVisionLogCategories.h"

ULocalTextureSampler::ULocalTextureSampler()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULocalTextureSampler::BeginPlay()
{
	Super::BeginPlay();

	if (!ShouldRunClientLogic())
	{
		UE_LOG(LOSVision, Verbose,
			TEXT("ULocalTextureSampler::BeginPlay >> Cannot run client logic (dedicated server)"));
		return;
	}

	// Setup will happen when SetLocalRenderTarget is called
}

void ULocalTextureSampler::UpdateLocalTexture()
{
	if (!ShouldRunClientLogic())
	{
		return;
	}
	
	if (!LocalMaskRT || !VisionSubsystem)
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
	UE_LOG(LOSVision, Log,
		TEXT("ULocalTextureSampler::SetLocalRenderTarget >> Called"));
	
	if (LocalMaskRT == InRT)
	{
		UE_LOG(LOSVision, Warning,
			TEXT("ULocalTextureSampler::SetLocalRenderTarget >> Already using same RT"));
		return;
	}

	LocalMaskRT = InRT;

	if (!InRT)
	{
		UE_LOG(LOSVision, Warning,
			TEXT("ULocalTextureSampler::SetLocalRenderTarget >> RT is null"));
		return;
	}

	if (!PrepareSetups())
	{
		UE_LOG(LOSVision, Error,
			TEXT("ULocalTextureSampler::SetLocalRenderTarget >> PrepareSetups failed"));
		return;
	}

	// Initial update
	UpdateLocalTexture();
}

bool ULocalTextureSampler::PrepareSetups()
{
	UE_LOG(LOSVision, Log,
		TEXT("ULocalTextureSampler::PrepareSetups >> Called"));
	
	// Grab the VisionSubsystem
	VisionSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UVisionSubsystem>() : nullptr;
	if (!VisionSubsystem)
	{
		UE_LOG(LOSVision, Warning,
			TEXT("ULocalTextureSampler::PrepareSetups >> Failed to get VisionSubsystem"));
		return false;
	}
	
	if (!CreateProjectionMID())
	{
		UE_LOG(LOSVision, Error,
			TEXT("ULocalTextureSampler::PrepareSetups >> Projection MID unavailable"));
		return false;
	}

	// Validate LocalMaskRT
	if (!LocalMaskRT)
	{
		UE_LOG(LOSVision, Warning,
			TEXT("ULocalTextureSampler::PrepareSetups >> LocalMaskRT not assigned"));
		return false;
	}

	// Resize DebugRT to match LocalMaskRT if enabled
	if (bDrawDebugRT && DebugRT)
	{
		if (DebugRT->SizeX != LocalMaskRT->SizeX || DebugRT->SizeY != LocalMaskRT->SizeY)
		{
			DebugRT->ResizeTarget(LocalMaskRT->SizeX, LocalMaskRT->SizeY);
			UE_LOG(LOSVision, Log,
				TEXT("ULocalTextureSampler::PrepareSetups >> Resized DebugRT to %dx%d"),
				DebugRT->SizeX, DebugRT->SizeY);
		}
	}

	return true;
}

bool ULocalTextureSampler::CreateProjectionMID()
{
	// Already created
	if (ProjectionMID)
	{
		return true;
	}

	if (!ProjectionMaterial)
	{
		UE_LOG(LOSVision, Error,
			TEXT("ULocalTextureSampler::CreateProjectionMID >> ProjectionMaterial not assigned"));
		return false;
	}

	ProjectionMID = UMaterialInstanceDynamic::Create(ProjectionMaterial, this);

	if (ProjectionMID)
	{
		UE_LOG(LOSVision, Log,
			TEXT("ULocalTextureSampler::CreateProjectionMID >> Successfully created MID"));
		return true;
	}

	UE_LOG(LOSVision, Error,
		TEXT("ULocalTextureSampler::CreateProjectionMID >> Failed to create MID"));
	return false;
}

bool ULocalTextureSampler::ShouldRunClientLogic() const
{
	if (GetNetMode() == NM_DedicatedServer)
		return false;

	return true;
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

	const TArray<FObstacleMaskTile>& Tiles = VisionSubsystem->GetTiles();
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
			ActiveTileIndices.Add(i);
		}
	}

	UE_LOG(LOSVision, Verbose,
		TEXT("ULocalTextureSampler::UpdateOverlappingTiles >> %d tiles in local area"),
		ActiveTileIndices.Num());
}

void ULocalTextureSampler::DrawTilesIntoLocalRT()
{
	// Validation
	if (!LocalMaskRT)
	{
		UE_LOG(LOSVision, Error,
			TEXT("ULocalTextureSampler::DrawTilesIntoLocalRT >> Missing render targets"));
		return;
	}

	/*if (!ProjectionMID)
	{
		UE_LOG(LOSVision, Error,
			TEXT("ULocalTextureSampler::DrawTilesIntoLocalRT >> ProjectionMID is null"));
		return;
	}*/

	if (!VisionSubsystem)
	{
		UE_LOG(LOSVision, Error,
			TEXT("ULocalTextureSampler::DrawTilesIntoLocalRT >> VisionSubsystem is null"));
		return;
	}

	// Clear accumulation buffer ONCE
	UKismetRenderingLibrary::ClearRenderTarget2D(
		this,
		LocalMaskRT,
		FLinearColor::Black);

	if (bDrawDebugRT && DebugRT)//do the same for the debug RT
	{
		UKismetRenderingLibrary::ClearRenderTarget2D(
			this,
			DebugRT,
			FLinearColor::Black);
	}

	if (ActiveTileIndices.IsEmpty())
	{
		UE_LOG(LOSVision, Verbose,
			TEXT("ULocalTextureSampler::DrawTilesIntoLocalRT >> No active tiles"));
		return;
	}

	const TArray<FObstacleMaskTile>& Tiles = VisionSubsystem->GetTiles();
	const FVector2D LocalSize = LocalWorldBounds.GetSize();
	const float CameraYawOffset = 90.f;

	UE_LOG(LOSVision, Verbose,
		TEXT("ULocalTextureSampler::DrawTilesIntoLocalRT >> Drawing %d tiles with accumulation"),
		ActiveTileIndices.Num());

	// using canvas item to draw caused the assert in multi--> now replace it with material
	// Initialize Canvas tools
	FCanvas Canvas(
		LocalMaskRT->GameThread_GetRenderTargetResource(),
		nullptr,
		GetWorld(),
		GMaxRHIFeatureLevel);
	
	// Scene capture camera rotation constant
	// Camera is at Rotation(-90, 90, 0) which creates a 90-degree transformation
	
	UE_LOG(LOSVision, Verbose,
		TEXT("ULocalTextureSampler::DrawTilesIntoLocalRT >> Drawing %d tiles"),
		ActiveTileIndices.Num());

	for (int32 TileIndex : ActiveTileIndices)
	{
		const FObstacleMaskTile& Tile = VisionSubsystem->GetTiles()[TileIndex];
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
		
		if (!Tile.Mask->GetResource())
		{
			UE_LOG(LOSVision, Verbose,
				TEXT("ULocalTextureSampler::DrawTilesIntoLocalRT >> Invalid Tile Texture Resource"));
			continue;
		}
		
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
		
	// Optionally draw debug RT for visualization
	if (bDrawDebugRT && DebugRT)
	{
		UKismetRenderingLibrary::DrawMaterialToRenderTarget(
			this,
			DebugRT,
			ProjectionMID);
	}

	UE_LOG(LOSVision, Verbose,
		TEXT("ULocalTextureSampler::DrawTilesIntoLocalRT >> Finished drawing tiles"));
}