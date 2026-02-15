// Fill out your copyright notice in the Description page of Project Settings.


#include "LineOfSight/WorldObstacle/LocalTextureSampler.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Canvas.h"
#include "Kismet/KismetRenderingLibrary.h"

#include "Engine/World.h"
#include "LineOfSight/Management/Subsystem/WorldObstacleSubsystem.h"
#include "TopDownVisionDebug.h"//log


//Log Helper here
// make it log the name of the local sampler obj



ULocalTextureSampler::ULocalTextureSampler()
{
	PrimaryComponentTick.bCanEverTick = false;
}


// Called when the game starts
void ULocalTextureSampler::BeginPlay()
{
	Super::BeginPlay();

	
	/*if (GetNetMode() == NM_DedicatedServer)
	{
		// Fully disable this component on server
		SetComponentTickEnabled(false);
		DestroyComponent();
		return;
	}*/

	if (!ShouldRunClientLogic())
	{
		return;// not for client logic here
	}

	PrepareSetups();
}

void ULocalTextureSampler::OnComponentCreated()
{
	Super::OnComponentCreated();

	
}

void ULocalTextureSampler::UpdateLocalTexture()
{
	if (!ShouldRunClientLogic())
	{
		return;// not for client logic here
	}
	
	if (!LocalMaskRT || !SourceRoot.IsValid())
	{
		UE_LOG(LOSVision, VeryVerbose,
			TEXT(" ULocalTextureSampler::UpdateLocalTexture >> skipped | RT=%d Root=%d"),
			LocalMaskRT != nullptr,
			SourceRoot.IsValid());
		return;
	}

	/*if (!ObstacleSubsystem)
	{
		UE_LOG(LOSVision, Verbose,
			TEXT("ULocalTextureSampler::UpdateLocalTexture >> Missing Subsystem"));
		return;
	}*/

	if (!ObstacleSubsystem)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			ObstacleSubsystem = World->GetSubsystem<UWorldObstacleSubsystem>();
            
			if (ObstacleSubsystem)
			{
				UE_LOG(LOSVision, Log,
					TEXT("UpdateLocalTexture >> Lazy-loaded VisionSubsystem with %d tiles"),
					ObstacleSubsystem->GetTiles().Num());
			}
		}
	}
    
	if (!ObstacleSubsystem)
	{
		UE_LOG(LOSVision, Warning,
			TEXT("UpdateLocalTexture >> Still missing Subsystem after lazy init"));
		return;
	}
	
	if (!SourceRoot.IsValid())
	{
		UE_LOG(LOSVision, Verbose,
					TEXT("ULocalTextureSampler::UpdateLocalTexture >> Missing SourceRoot"));
		return;
	}
	
	const FVector WorldCenter = SourceRoot->GetComponentLocation();
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

void ULocalTextureSampler::SetLocalRenderTarget(UTextureRenderTarget2D* InRT)// this is when the prep should be made
{
	if (!ShouldRunClientLogic())
	{
		return;// not for client logic here
	}
	
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
	ObstacleSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UWorldObstacleSubsystem>() : nullptr;
	if (!ObstacleSubsystem)
	{
		UE_LOG(LOSVision, Warning, TEXT("ULocalTextureSampler::PrepareSetups >> Failed to get VisionSubsystem"));
	}
}

bool ULocalTextureSampler::ShouldRunClientLogic() const
{
	if (GetNetMode() == NM_DedicatedServer)
		return false;

	return true;
}

void ULocalTextureSampler::SetLocationRoot(USceneComponent* NewRoot)
{
	if (!NewRoot)
	{
		UE_LOG(LOSVision, Error,
			TEXT(" ULocalTextureSampler::SetAsOwnerRoot >> Invalid Root"))
		return;
	}

	SourceRoot = NewRoot;
	UE_LOG(LOSVision, Log,
		TEXT(" ULocalTextureSampler::SetAsOwnerRoot >> Root Settled"))
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

		const TCHAR* TextureName =Tiles[i].Mask? *Tiles[i].Mask->GetName(): TEXT("None");
		
		UE_LOG(LOSVision, VeryVerbose,
			TEXT("Tile %d | Texture=%s | TileBounds [%s - %s] | Overlap=%d"),
			i,
			TextureName,
			*Tiles[i].WorldBounds.Min.ToString(),
			*Tiles[i].WorldBounds.Max.ToString(),
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

	//clear the debug RT as well

	if (bDrawDebugRT && DebugRT)
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

	// Initialize Canvas tools
	FCanvas Canvas(
		LocalMaskRT->GameThread_GetRenderTargetResource(),
		nullptr,
		GetWorld(),
		GMaxRHIFeatureLevel);
	
	//f0r debug canvas
	FCanvas* DebugCanvas = nullptr;

	if (bDrawDebugRT && DebugRT)
	{
		DebugCanvas = new FCanvas(
			DebugRT->GameThread_GetRenderTargetResource(),
			nullptr,
			GetWorld(),
			GMaxRHIFeatureLevel);
	}

	
	const FVector2D LocalSize = LocalWorldBounds.GetSize();
	
	// Scene capture camera rotation constant
	// Camera is at Rotation(-90, 90, 0) which creates a 90-degree transformation
	const float CameraYawOffset = 90.f;

	UE_LOG(LOSVision, Verbose,
		TEXT("ULocalTextureSampler::DrawTilesIntoLocalRT >> Drawing %d tiles"),
		ActiveTileIndices.Num());

	/*for (int32 TileIndex : ActiveTileIndices)
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

		//f0r debug canvas
		if (bDrawDebugRT && DebugRT)
		{
			DebugCanvas->DrawItem(TileItem);
		}
	}
	*/

	for (int32 TileIndex : ActiveTileIndices)
	{
		const FObstacleMaskTile& Tile = ObstacleSubsystem->GetTiles()[TileIndex];
		if (!Tile.Mask) continue;

		// --- Math Section (Your original logic) ---
		FVector2D LocalRelative = Tile.WorldCenter - LocalWorldBounds.Min;
		FVector2D RotatedLocal;
		RotatedLocal.X = LocalRelative.Y;
		RotatedLocal.Y = LocalSize.X - LocalRelative.X;
        
		FVector2D TileCenterInRT;
		TileCenterInRT.X = (RotatedLocal.X / LocalSize.Y) * LocalMaskRT->SizeX;
		TileCenterInRT.Y = (RotatedLocal.Y / LocalSize.X) * LocalMaskRT->SizeY;
        
		FVector2D TileSizeInRT;
		TileSizeInRT.X = (Tile.WorldSize.Y / LocalSize.Y) * LocalMaskRT->SizeX;
		TileSizeInRT.Y = (Tile.WorldSize.X / LocalSize.X) * LocalMaskRT->SizeY;

		// --- THE FIX: Inset Rendering ---
		// We subtract a tiny fraction (0.5 pixel) from the size to prevent 
		// the sampler from touching the "bleeding" edge of the source texture.
		FVector2D SafeSize = TileSizeInRT - FVector2D(0.5f, 0.5f);
		FVector2D TilePosInRT = TileCenterInRT - (SafeSize * 0.5f);

		FCanvasTileItem TileItem(TilePosInRT, Tile.Mask->GetResource(), SafeSize, FLinearColor::White);
        
		// Ensure we use Additive blending so overlapping tiles merge correctly
		TileItem.BlendMode = SE_BLEND_Additive;
        
		// --- THE FIX: UV Manual Clamp ---
		// Instead of 0 to 1, we sample 0.001 to 0.999
		TileItem.UV0 = FVector2D(0.001f, 0.001f);
		TileItem.UV1 = FVector2D(0.999f, 0.999f);

		TileItem.PivotPoint = FVector2D(0.5f, 0.5f);
		TileItem.Rotation = FRotator(0.f, Tile.WorldRotationYaw - CameraYawOffset, 0.f);
        
		Canvas.DrawItem(TileItem);
	}

	//  Tell the GPU to execute all queued draws at once!!!!
	Canvas.Flush_GameThread();

	if (DebugCanvas)//draw debug canvas
	{
		DebugCanvas->Flush_GameThread();
		delete DebugCanvas;
	}

	UE_LOG(LOSVision, Verbose,
		TEXT("ULocalTextureSampler::DrawTilesIntoLocalRT >> Finished drawing tiles"));
}
