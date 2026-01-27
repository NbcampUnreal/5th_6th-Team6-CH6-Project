// Fill out your copyright notice in the Description page of Project Settings.


#include "LineOfSight/CameraVisionManager.h"
#include "LineOfSight/LineOfSightComponent.h"// the local LOS stamps
#include "Engine/World.h"
#include "Engine/Canvas.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"// MID of local LOS stamps
#include "TopDownVisionLogCategories.h"//log
#include "Materials/MaterialParameterCollectionInstance.h"

void UCameraVisionManager::Initialize(APlayerCameraManager* InCamera)
{
	ActiveCamera = InCamera;

	if (!CameraLocalRT && ActiveCamera)// to use it for minimap shared vision, make it dynamically
	{
		// Create the Canvas Render Target
		CameraLocalRT = UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(
			ActiveCamera->GetWorld(),
			UCanvasRenderTarget2D::StaticClass(),
			RTSize,
			RTSize
		);

		if (CameraLocalRT)
		{
			CameraLocalRT->OnCanvasRenderTargetUpdate.AddDynamic(this, &UCameraVisionManager::DrawLOS);
			CameraLocalRT->UpdateResource(); // initial draw
			UE_LOG(LOSVision, Log,
				TEXT("UCameraVisionManager::Initialize >> CameraLocalRT created"));
		}
	}
}

void UCameraVisionManager::RegisterLOSProvider(ULineOfSightComponent* Provider)
{
	if (Provider && !LOSProviders.Contains(Provider))
	{
		LOSProviders.Add(Provider);
		bDirty = true;
		UE_LOG(LOSVision, Log,
			TEXT("UCameraVisionManager::RegisterLOSProvider >> Provider registered"));
	}
}

void UCameraVisionManager::UnregisterLOSProvider(ULineOfSightComponent* Provider)
{
	if (Provider)
	{
		LOSProviders.Remove(Provider);
		bDirty = true;
		UE_LOG(LOSVision, Log,
			TEXT("UCameraVisionManager::UnregisterLOSProvider >> Provider unregistered"));
	}
}

void UCameraVisionManager::UpdateCameraLOS()
{
	if (!CameraLocalRT || !bDirty)
		return;
	
	// First, toggle providers based on visibility
	for (ULineOfSightComponent* Provider : LOSProviders)
	{
		if (!Provider || !Provider->GetOwner())
			continue;

		FVector2D PixelPos;
		float TileSize;
		bool bVisible = ConvertWorldToRT(
			Provider->GetOwner()->GetActorLocation(),
			Provider->GetVisibleRange(),
			PixelPos,
			TileSize);

		// Enable update only if visible
		Provider->ToggleUpdate(bVisible);

		if (bVisible)
		{
			// Update MID parameters inside provider
			Provider->UpdateMID();
		}
	}

	// Now update the camera RT, triggers DrawLOS
	CameraLocalRT->UpdateResource();
	bDirty = false;
}

void UCameraVisionManager::SetActiveCamera(APlayerCameraManager* InCamera)
{
	if (!InCamera || InCamera == ActiveCamera)
		return;

	ActiveCamera = InCamera;

	// Mark RT dirty to redraw from new camera perspective
	bDirty = true;
}

void UCameraVisionManager::DrawLOS(UCanvas* Canvas, int32 Width, int32 Height)
{
	if (!Canvas || !CameraLocalRT)
	{
		UE_LOG(LOSVision, Warning,
			TEXT("UCameraVisionManager::DrawLOS >> Canvas or CameraLocalRT is null"));
		return;
	}

	// Clear canvas
	Canvas->K2_DrawTexture(
		nullptr,//default black
		FVector2D(0, 0),
		FVector2D(Width, Height),
		FVector2D(0, 0));

	int32 CompositedCount = 0;// for logging

	for (ULineOfSightComponent* Provider : LOSProviders)
	{
		if (!Provider || !Provider->IsUpdating())
			continue;

		UMaterialInstanceDynamic* MID = Provider->GetLOSMaterialMID();
		if (!MID)
			continue;

		FVector2D PixelPos;
		float TileSize;
		if (!ConvertWorldToRT(
			Provider->GetOwner()->GetActorLocation(),
			Provider->GetVisibleRange(),
			PixelPos,
			TileSize))
			continue;// if it failed to convert to world-> don't draw it

		FCanvasTileItem Tile(
			PixelPos - FVector2D(TileSize * 0.5f, TileSize * 0.5f),// to the center pivot
			MID->GetRenderProxy(),
			FVector2D(TileSize, TileSize)
		);
		Tile.BlendMode = SE_BLEND_AlphaComposite;// layer using alpha mask
		Canvas->DrawItem(Tile);//stack the image by drawing over prev

		CompositedCount++;//increment draw count
	}

	UE_LOG(LOSVision, Log,
		TEXT("UCameraVisionManager::DrawLOS >> Camera RT composited %d providers"),
		CompositedCount);
}

bool UCameraVisionManager::ConvertWorldToRT(const FVector& ProviderWorldLocation, const float& ProviderVisionRange,
	FVector2D& OutPixelPosition, float& OutTileSize) const
{
	if (!ActiveCamera || !CameraLocalRT)
	{
		UE_LOG(LOSVision, Error,
		TEXT("UCameraVisionManager::ConvertWorldToRT >> Invalid ActiveCamera or CameraLocalRT"));
		return false;
	}

	FVector CameraLocation = ActiveCamera->GetCameraLocation();

	FVector Delta = ProviderWorldLocation - CameraLocation;// get normalized value to be used for getting location of stamps

	// CameraVisionRange is half-width of the RT in world units
	// normalize [-CameraVisionRange, CameraVisionRange] -> [0,1]
	float NormalizedX = 0.5f + (Delta.X / CameraVisionRange);
	float NormalizedY = 0.5f + (Delta.Y / CameraVisionRange);

	// Check if the provider is inside camera view
	if (NormalizedX < 0.f || NormalizedX > 1.f || NormalizedY < 0.f || NormalizedY > 1.f)
	{
		UE_LOG(LOSVision, Error,
		TEXT("UCameraVisionManager::ConvertWorldToRT >> provider is out of range"));
		return false;
	}

	// Pixel coordinates on the RT
	OutPixelPosition.X = NormalizedX * CameraLocalRT->SizeX;
	OutPixelPosition.Y = NormalizedY * CameraLocalRT->SizeY;

	// Tile size in pixels based on provider's vision range
	OutTileSize = FMath::Max(4.f, (ProviderVisionRange / CameraVisionRange) * CameraLocalRT->SizeX);

	return true;
}
