#include "LineOfSight/CameraVisionManager.h"
#include "LineOfSight/LineOfSightComponent.h"
#include "Engine/World.h"
#include "Engine/Canvas.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "TopDownVisionLogCategories.h"
#include "LineOfSight/VisionSubsystem.h"

UCameraVisionManager::UCameraVisionManager()
{
	VisionChannel = INDEX_NONE;
	CameraVisionRange = 1000.f;
	RTSize = 1024;
	bDirty = true;

	UE_LOG(LOSVision, Log, TEXT("UCameraVisionManager::Constructor >> Component constructed"));
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
	bDirty = true;

	UE_LOG(LOSVision, Log, TEXT("UCameraVisionManager::Initialize >> Initialized with CameraLocalRT: %s"), *CameraLocalRT->GetName());
}

void UCameraVisionManager::UpdateCameraLOS()
{
	UE_LOG(LOSVision, Log, TEXT("UCameraVisionManager::UpdateCameraLOS >> Called, bDirty=%s"), bDirty ? TEXT("true") : TEXT("false"));

	if (!CameraLocalRT)
	{
		UE_LOG(LOSVision, Error, TEXT("CameraLocalRT is null!"));
		return;
	}
	if (!bDirty)
	{
		UE_LOG(LOSVision, Log, TEXT("Skipping update because bDirty is false"));
		return;
	}

	UVisionSubsystem* Subsystem = GetWorld()->GetSubsystem<UVisionSubsystem>();
	if (!Subsystem)
	{
		UE_LOG(LOSVision, Error, TEXT("VisionSubsystem not found!"));
		return;
	}

	const TArray<ULineOfSightComponent*>& ActiveProviders = Subsystem->GetProvidersForTeam(VisionChannel);
	UE_LOG(LOSVision, Log, TEXT("Found %d active LOS providers for channel %d"), ActiveProviders.Num(), VisionChannel);

	// Toggle provider updates based on visibility
	for (ULineOfSightComponent* Provider : ActiveProviders)
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

		Provider->ToggleUpdate(bVisible);
		UE_LOG(LOSVision, Log, TEXT("Provider %s visibility set to %s"), *Provider->GetOwner()->GetName(), bVisible ? TEXT("true") : TEXT("false"));
	}

	// Draw all providers to the RT
	CameraLocalRT->UpdateResource();
	bDirty = false;
	UE_LOG(LOSVision, Log, TEXT("CameraLocalRT UpdateResource called"));

	// Optional: update MPC scalars (camera location, size)
	if (PostProcessMPC && ActiveCamera)
	{
		UMaterialParameterCollectionInstance* MPCInstance = GetWorld()->GetParameterCollectionInstance(PostProcessMPC);
		if (MPCInstance)
		{
			FVector WorldLocation = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;

			MPCInstance->SetVectorParameterValue(
				CenterLocationParam,
				FLinearColor(WorldLocation.X, WorldLocation.Y, WorldLocation.Z));

			MPCInstance->SetScalarParameterValue(
				VisibleRangeParam,
				CameraVisionRange * 2);

			UE_LOG(LOSVision, Log, TEXT("PostProcessMPC updated: CenterLocation=%s, VisibleRange=%f"),
				*WorldLocation.ToString(), CameraVisionRange*2);
		}
		else
		{
			UE_LOG(LOSVision, Warning, TEXT("PostProcessMPC instance not found!"));
		}
	}

	UE_LOG(LOSVision, Log, TEXT("UCameraVisionManager::UpdateCameraLOS >> Update finished"));
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
	bDirty = true;
}

void UCameraVisionManager::DrawLOS(UCanvas* Canvas, int32 Width, int32 Height)
{
	UE_LOG(LOSVision, Log, TEXT("DrawLOS called: Canvas=%s, Width=%d, Height=%d"), *GetNameSafe(Canvas), Width, Height);

	if (!Canvas || !CameraLocalRT)
	{
		UE_LOG(LOSVision, Warning,
			TEXT("DrawLOS skipped: Canvas or CameraLocalRT is null"));
		return;
	}

	// Clear canvas
	Canvas->K2_DrawTexture(nullptr, FVector2D(0, 0), FVector2D(Width, Height), FVector2D(0, 0));

	UVisionSubsystem* Subsystem = GetWorld()->GetSubsystem<UVisionSubsystem>();
	if (!Subsystem)
	{
		UE_LOG(LOSVision, Warning, TEXT("DrawLOS skipped: VisionSubsystem not found"));
		return;
	}

	const TArray<ULineOfSightComponent*>& ActiveProviders = Subsystem->GetProvidersForTeam(VisionChannel);
	int32 CompositedCount = 0;

	for (ULineOfSightComponent* Provider : ActiveProviders)
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
			continue;

		FCanvasTileItem Tile(
			PixelPos - FVector2D(TileSize * 0.5f, TileSize * 0.5f),
			MID->GetRenderProxy(),
			FVector2D(TileSize, TileSize));
		Tile.BlendMode = SE_BLEND_AlphaComposite;
		Canvas->DrawItem(Tile);

		CompositedCount++;
	}

	UE_LOG(LOSVision, Log, TEXT("DrawLOS >> Composited %d providers"), CompositedCount);
}

bool UCameraVisionManager::ConvertWorldToRT(const FVector& ProviderWorldLocation, const float& ProviderVisionRange, FVector2D& OutPixelPosition, float& OutTileSize) const
{
	if (!ActiveCamera || !CameraLocalRT)
	{
		UE_LOG(LOSVision, Error, TEXT("ConvertWorldToRT failed: invalid ActiveCamera or CameraLocalRT"));
		return false;
	}

	FVector Delta = ProviderWorldLocation - GetOwner()->GetActorLocation();

	float NormalizedX = 0.5f + (Delta.X / CameraVisionRange);
	float NormalizedY = 0.5f + (Delta.Y / CameraVisionRange);

	if (NormalizedX < 0.f || NormalizedX > 1.f || NormalizedY < 0.f || NormalizedY > 1.f)
	{
		UE_LOG(LOSVision, Log, TEXT("ConvertWorldToRT >> Provider out of range: %s"), *ProviderWorldLocation.ToString());
		return false;
	}

	OutPixelPosition.X = NormalizedX * CameraLocalRT->SizeX;
	OutPixelPosition.Y = NormalizedY * CameraLocalRT->SizeY;
	OutTileSize = FMath::Max(4.f, (ProviderVisionRange / CameraVisionRange) * CameraLocalRT->SizeX);

	UE_LOG(LOSVision, Log, TEXT("ConvertWorldToRT >> PixelPos=(%.2f, %.2f), TileSize=%.2f"), OutPixelPosition.X, OutPixelPosition.Y, OutTileSize);

	return true;
}
