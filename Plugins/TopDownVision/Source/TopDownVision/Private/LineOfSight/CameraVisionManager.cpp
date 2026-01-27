// Fill out your copyright notice in the Description page of Project Settings.


#include "LineOfSight/CameraVisionManager.h"
#include "LineOfSight/LineOfSightComponent.h"
#include "Engine/World.h"
#include "Engine/Canvas.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TopDownVisionLogCategories.h"//log
#include "Materials/MaterialParameterCollectionInstance.h"

void UCameraVisionManager::Initialize(APlayerCameraManager* InCamera)
{
	if (!InCamera)
		return;

	ActiveCamera = InCamera;

	// Create the shared camera-local Canvas Render Target
	if (!CameraLocalRT && ActiveCamera->GetWorld())
	{
		CameraLocalRT = UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(
			ActiveCamera->GetWorld(),
			UCanvasRenderTarget2D::StaticClass(),
			RTSize,
			RTSize
		);

		if (CameraLocalRT)
		{
			// Bind the draw callback
			CameraLocalRT->OnCanvasRenderTargetUpdate.AddDynamic(this, &UCameraVisionManager::DrawLOS);

			// Initial draw
			CameraLocalRT->UpdateResource();
		}
	}
}

void UCameraVisionManager::RegisterLOSProvider(ULineOfSightComponent* Provider)
{
	if (!Provider || LOSProviders.Contains(Provider))
		return;

	LOSProviders.Add(Provider);
	bDirty = true;
}

void UCameraVisionManager::UnregisterLOSProvider(ULineOfSightComponent* Provider)
{
	if (!Provider)
		return;

	LOSProviders.Remove(Provider);
	bDirty = true;
}

void UCameraVisionManager::UpdateCameraLOS()
{
	if (!CameraLocalRT || !ActiveCamera || !bDirty)
		return;

	// Triggers DrawLOS callback
	CameraLocalRT->UpdateResource();

	// Clear dirty flag after update
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
	if (!Canvas || !LOSMaterial)
		return;

	// Clear canvas to black / full hidden
	Canvas->K2_DrawTexture(
		nullptr,// nullptr = black default
		FVector2D(0, 0), // Top-left
		FVector2D(Width, Height),
		FVector2D(0, 0)// UV offset
	);

	// Draw each registered LOS provider
	for (ULineOfSightComponent* Provider : LOSProviders)
	{
		if (!Provider || !Provider->GetOwner())
			continue;

		FVector ActorLocation = Provider->GetOwner()->GetActorLocation();



		// Draw the LOS material over the RT
		FCanvasTileItem Tile(
			FVector2D(0, 0),                       // full RT coverage
			LOSMaterial->GetRenderProxy(),
			FVector2D(Width, Height)
		);
		Tile.BlendMode = SE_BLEND_Opaque;          // full mask
		Canvas->DrawItem(Tile);
	}
}
