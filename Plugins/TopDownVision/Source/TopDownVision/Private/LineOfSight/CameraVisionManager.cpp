#include "LineOfSight/CameraVisionManager.h"
#include "LineOfSight/LineOfSightComponent.h"
#include "Engine/World.h"
#include "Engine/Canvas.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "TopDownVisionLogCategories.h"
#include "LineOfSight/VisionSubsystem.h"


//Internal helper



UCameraVisionManager::UCameraVisionManager()
{
	UE_LOG(LOSVision, Log,
		TEXT("UCameraVisionManager::Constructor >> Component constructed"));
}

void UCameraVisionManager::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LOSVision, Log, TEXT("UCameraVisionManager::BeginPlay >> BeginPlay called"));
}

void UCameraVisionManager::Initialize(APlayerCameraManager* InCamera)
{
	ActiveCamera = InCamera;
	UE_LOG(LOSVision, Log, TEXT("UCameraVisionManager::Initialize >> Called with Camera: %s"), *GetNameSafe(ActiveCamera));

	if (!CameraLocalRT)
	{
		UE_LOG(LOSVision, Warning,
			TEXT("UCameraVisionManager::Initialize >> CameraLocalRT is null. Assign it in the Content Browser."));
		return;
	}

	// Bind the draw callback
	CameraLocalRT->OnCanvasRenderTargetUpdate.AddDynamic(this, &UCameraVisionManager::DrawLOS);
	CameraLocalRT->UpdateResource(); // Initial draw

	UE_LOG(LOSVision, Log, TEXT("UCameraVisionManager::Initialize >> Initialized with CameraLocalRT: %s"),
		*CameraLocalRT->GetName());

	//initialize the world location and texture size
	MPCInstance = GetWorld()->GetParameterCollectionInstance(PostProcessMPC);
	if (MPCInstance)
	{
		FVector WorldLocation = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;

		MPCInstance->SetVectorParameterValue(
			MPCLocationParam,
			FLinearColor(WorldLocation.X, WorldLocation.Y, WorldLocation.Z));

		MPCInstance->SetScalarParameterValue(
			MPCTextureSizeParam,
			RTSize);

		MPCInstance->SetScalarParameterValue(
			MPCVisibleRangeParam,
			CameraVisionRange);
	}
	else
	{
		UE_LOG(LOSVision, Warning,
			TEXT("UCameraVisionManager::UpdateCameraLOS >> PostProcessMPC instance not found!"));
	}

	//Make a MID which will draw the Layered CRT
	if (LayeredLOSInterfaceMaterial)
	{
		LayeredLOSInterfaceMID =
			UMaterialInstanceDynamic::Create(LayeredLOSInterfaceMaterial, this);

		if (LayeredLOSInterfaceMID)
		{
			LayeredLOSInterfaceMID->SetTextureParameterValue(
				LayeredLOSTextureParam,
				CameraLocalRT
			);
		}
	}
	else
	{
		UE_LOG(LOSVision, Warning,
			TEXT("UCameraVisionManager::UpdateCameraLOS >> CameraLOSInterfaceMaterial is not assigned"));
	}
}

//LayeredStamps


//internal helper for checking overlap
static bool RectOverlapsWorld(
	const FVector& ACenter,
	float AHalfSize,
	const FVector& BCenter,
	float BHalfSize)
{
	return !(
		FMath::Abs(ACenter.X - BCenter.X) > (AHalfSize + BHalfSize) ||
		FMath::Abs(ACenter.Y - BCenter.Y) > (AHalfSize + BHalfSize)
	);
}

void UCameraVisionManager::UpdateCameraLOS()
{
	UE_LOG(LOSVision, Log,
		TEXT("UCameraVisionManager::UpdateCameraLOS >> Called"));

	if (!CameraLocalRT)
	{
		UE_LOG(LOSVision, Error,
			TEXT("UCameraVisionManager::UpdateCameraLOS >>CameraLocalRT is null!"));
		return;
	}

	TArray<ULineOfSightComponent*> ActiveProviders;//catchers
	if (!GetVisibleProviders(ActiveProviders))
	{
		UE_LOG(LOSVision, Error,
			TEXT("UCameraVisionManager::DrawLOS >> Failed to bring VisibleProviders"));
		return;
	}

	const FVector CameraCenter = GetOwner()->GetActorLocation();

	for (ULineOfSightComponent* Provider : ActiveProviders)
	{
		if (!Provider || !Provider->GetOwner())
			continue;

		const bool bVisible = RectOverlapsWorld(
			CameraCenter,
			CameraVisionRange,
			Provider->GetOwner()->GetActorLocation(),
			Provider->GetVisibleRange());

		Provider->ToggleUpdate(bVisible);
	}

	// Draw all providers to the RT
	CameraLocalRT->UpdateResource();
	UE_LOG(LOSVision, Log,
		TEXT("UCameraVisionManager::UpdateCameraLOS >> CameraLocalRT UpdateResource called"));

	// update MPC scalars (camera location, size)
	if (MPCInstance)
	{
		FVector WorldLocation = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;

		MPCInstance->SetVectorParameterValue(
			MPCLocationParam,
			FLinearColor(WorldLocation.X, WorldLocation.Y, WorldLocation.Z));

		UE_LOG(LOSVision, Log,
			TEXT("UCameraVisionManager::UpdateCameraLOS >> CenterLocation=%s, VisibleRange=%f"),
			*WorldLocation.ToString(), CameraVisionRange);
	}
	else
	{
		UE_LOG(LOSVision, Warning,
			TEXT("UCameraVisionManager::UpdateCameraLOS >> PostProcessMPC instance not found!"));
	}

	UE_LOG(LOSVision, Log,
		TEXT("UCameraVisionManager::UpdateCameraLOS >> Update finished"));
}

void UCameraVisionManager::SetActiveCamera(APlayerCameraManager* InCamera)
{
	UE_LOG(LOSVision, Log, TEXT("UCameraVisionManager::SetActiveCamera >> Called with %s"), *GetNameSafe(InCamera));

	if (!InCamera || InCamera == ActiveCamera)
	{
		UE_LOG(LOSVision, Log, TEXT("SetActiveCamera skipped: invalid or same camera"));
		return;
	}

	ActiveCamera = InCamera;
}

void UCameraVisionManager::DrawLOS(UCanvas* Canvas, int32 Width, int32 Height)
{
	UE_LOG(LOSVision, Log,
		TEXT("UCameraVisionManager::DrawLOS >> Canvas=%s, Width=%d, Height=%d"),
		*GetNameSafe(Canvas), Width, Height);

	if (!Canvas || !CameraLocalRT)
	{
		UE_LOG(LOSVision, Warning,
			TEXT("UCameraVisionManager::DrawLOS >> Canvas or CameraLocalRT is null"));
		return;
	}

	// Clear canvas
	Canvas->K2_DrawTexture(
		nullptr,
		FVector2D(0, 0),
		FVector2D(Width, Height),
		FVector2D(0, 0));

	TArray<ULineOfSightComponent*> ActiveProviders;//catchers
	if (!GetVisibleProviders(ActiveProviders))
	{
		UE_LOG(LOSVision, Error,
			TEXT("UCameraVisionManager::DrawLOS >> Failed to bring VisibleProviders"));
		return;
	}
	int32 CompositedCount = 0;

	for (ULineOfSightComponent* Provider : ActiveProviders)
	{
		if (!Provider || !Provider->IsUpdating())
			continue;

		LOSMaterialInstance = Provider->GetLOSMaterialMID();
		if (!LOSMaterialInstance)
			continue;

		//Catcher
		FVector2D PixelPos;
		float TileSize;
		
		if (!ConvertWorldToRT(
			Provider->GetOwner()->GetActorLocation(),
			Provider->GetVisibleRange(),
			//outputs
			PixelPos,
			TileSize))
			continue;

		FCanvasTileItem Tile(
			PixelPos - FVector2D(TileSize * 0.5f, TileSize * 0.5f),// set the pivot to the center of the texture
			LOSMaterialInstance->GetRenderProxy(),//handle for the rendering
			FVector2D(TileSize, TileSize));//size of the texture(always square)

		//Blend Mode for drawing over canvas
		Tile.BlendMode = SE_BLEND_AlphaComposite;//draw over with alpha mask
		//Tile.BlendMode = SE_BLEND_Additive;//testing
		
		Canvas->DrawItem(Tile);//draw the tile over the canvas

		CompositedCount++;//increment the count
	}


	if (bDrawTextureRange)//draw debug box for LOS stamp area
	{
		const FVector Center = GetOwner()->GetActorLocation();
		const FVector Extent = FVector(CameraVisionRange, CameraVisionRange, 50.f);

		DrawDebugBox(
			GetWorld(),
			Center,
			Extent,
			FQuat::Identity,
			FColor::Green,
			false,
			-1.f,
			0,
			2.f );
	}

	UE_LOG(LOSVision, Log,
		TEXT("UCameraVisionManager::DrawLOS >> Composited %d providers"),
		CompositedCount);
}

bool UCameraVisionManager::ConvertWorldToRT(const FVector& ProviderWorldLocation, const float& ProviderVisionRange, FVector2D& OutPixelPosition, float& OutTileSize) const
{
	if (!CameraLocalRT)
		return false;

	FVector Delta = ProviderWorldLocation - GetOwner()->GetActorLocation();

	float NormalizedX = 0.5f + (Delta.X / CameraVisionRange);
	float NormalizedY = 0.5f + (Delta.Y / CameraVisionRange);

	OutPixelPosition.X = NormalizedX * CameraLocalRT->SizeX;
	OutPixelPosition.Y = NormalizedY * CameraLocalRT->SizeY;

	OutTileSize = (ProviderVisionRange / CameraVisionRange) * CameraLocalRT->SizeX;
	OutTileSize = FMath::Max(4.f, OutTileSize);

	return true;
}

bool UCameraVisionManager::GetVisibleProviders(TArray<ULineOfSightComponent*>& OutProviders) const
{
	if (VisionChannel==INDEX_NONE)
	{
		UE_LOG(LOSVision, Error,
			TEXT("UCameraVisionManager::GetVisibleProviders >> Invalid VisionChannel"));
		return false;
	}
	UVisionSubsystem* Subsystem = GetWorld()->GetSubsystem<UVisionSubsystem>();
	if (!Subsystem)
	{
		UE_LOG(LOSVision, Error,
			TEXT("UCameraVisionManager::GetVisibleProviders >> VisionSubsystem not found"));
		return false;
	}
	//Get the Teams
	OutProviders = Subsystem->GetProvidersForTeam(VisionChannel);
	return true;
}




