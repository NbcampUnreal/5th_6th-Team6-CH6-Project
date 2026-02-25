#include "LineOfSight/CameraVisionManager.h"

#include "RenderGraphUtils.h"
#include "LineOfSight/LineOfSightComponent.h"
#include "Engine/World.h"
#include "Engine/Canvas.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "TopDownVisionDebug.h"
#include "LineOfSight/GPU/LOSStampPass.h"
#include "LineOfSight/Management/Subsystem/LOSVisionSubsystem.h"

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

void UCameraVisionManager::Initialize()
{
	if (!ShouldRunClientLogic())
	{
		return;// not for server
	}

	if (!CameraLocalRT)
	{
		UE_LOG(LOSVision, Warning,
			TEXT("UCameraVisionManager::Initialize >> CameraLocalRT is null. Assign it in the Content Browser."));
		return;
	}

	// Bind the draw callback if CPU
	if (bUseCPU)
	{
		CameraLocalRT->OnCanvasRenderTargetUpdate.AddDynamic(this, &UCameraVisionManager::DrawLOS_CPU);
		CameraLocalRT->UpdateResource(); // initial clear

		UE_LOG(LOSVision, Log, TEXT("UCameraVisionManager::Initialize >> Initialized with CameraLocalRT: %s"),
		*CameraLocalRT->GetName());
	}

	//initialize the world location and texture size
	MPCInstance = GetWorld()->GetParameterCollectionInstance(PostProcessMPC);
	if (MPCInstance)
	{
		FVector WorldLocation = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;

		MPCInstance->SetVectorParameterValue(
			MPCLocationParam,
			FLinearColor(WorldLocation.X, WorldLocation.Y, WorldLocation.Z));

		/*MPCInstance->SetScalarParameterValue(
			MPCTextureSizeParam,
			RTSize);*/ // fuck off

		MPCInstance->SetScalarParameterValue(
			MPCVisibleRangeParam,
			CameraVisionRange);
	}
	else
	{
		//UE_LOG(LOSVision, Warning, TEXT("UCameraVisionManager::UpdateCameraLOS >> PostProcessMPC instance not found!"));
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
		//UE_LOG(LOSVision, Warning, TEXT("UCameraVisionManager::UpdateCameraLOS >> CameraLOSInterfaceMaterial is not assigned"));
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
	if (!ShouldRunClientLogic())
	{
		return;// not for server
	}

	/*UE_LOG(LOSVision, Log,
		TEXT("UCameraVisionManager::UpdateCameraLOS >> Called"));*/

	if (!CameraLocalRT)
	{
		UE_LOG(LOSVision, Error,
			TEXT("UCameraVisionManager::UpdateCameraLOS >>CameraLocalRT is null!"));
		return;
	}

	// gather provider visibility update
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

		Provider->ToggleLOSStampUpdate(bVisible);

		if (bVisible)// !!!!! FUCK YEAH !!!!!!
		{
			Provider->UpdateLocalLOS();// update in here, not in the owner's tick update
		}
	}

	if (bUseCPU)// if CPU
	{
		// Draw all providers to the RT
		CameraLocalRT->UpdateResource();
		/*UE_LOG(LOSVision, Log,
			TEXT("UCameraVisionManager::UpdateCameraLOS >> CameraLocalRT UpdateResource called"));*/
	}
	else //GPU
	{
		// ---- Build GPU stamp data on GAME THREAD ----
		TArray<FLOSStampData> StampData_GT;
		StampData_GT.Reserve(ActiveProviders.Num());

		for (ULineOfSightComponent* Provider : ActiveProviders)
		{
			if (!Provider || !Provider->IsUpdating())
				continue;

			AActor* Owner = Provider->GetOwner();
			if (!Owner)
				continue;

			const FVector WorldPos = Owner->GetActorLocation();

			// Normalize into camera LOS texture space
			const float MaxRange = CameraVisionRange;

			const FVector2f CenterUV(
				(WorldPos.X - CameraCenter.X) / (MaxRange * 2.f) + 0.5f,
				(WorldPos.Y - CameraCenter.Y) / (MaxRange * 2.f) + 0.5f
			);

			const float RadiusUV = Provider->GetVisibleRange() / (MaxRange * 2.f);

			// Convert EVisionChannel -> bitmask
			const EVisionChannel V_Channel = Provider->GetVisionChannel();
			const uint32 ChannelBitMask =
				(V_Channel == EVisionChannel::None)
					? 0u
					: (1u << static_cast<uint32>(V_Channel));

			FLOSStampData& Stamp = StampData_GT.AddDefaulted_GetRef();
			Stamp.CenterRadiusStrength = FVector4f(
				CenterUV.X,
				CenterUV.Y,
				RadiusUV,
				1.0f            // strength (can be parameterized later)
			);
			Stamp.ChannelBitMask = ChannelBitMask;
		}

		// Copy for render thread safety
		const TArray<FLOSStampData> StampData_RT = StampData_GT;
		const uint32 ViewMask = CameraViewChannelMask;

		ENQUEUE_RENDER_COMMAND(UpdateLOS_GPU)(
			[this, StampData_RT, ViewMask](FRHICommandListImmediate& RHICmdList)
			{
				FRDGBuilder GraphBuilder(RHICmdList);

				FRDGTextureRef LOSTexture =
					RegisterExternalTexture(
						GraphBuilder,
						CameraLocalRT->GetRenderTargetResource()->GetRenderTargetTexture(),
						TEXT("CameraLOS_GPU"));

				AddLOSStampPass(
					GraphBuilder,
					LOSTexture,
					StampData_RT,
					ViewMask,
					true // clear before stamping
				);

				GraphBuilder.Execute();
			});
	}



	// update MPC scalars (camera location, size)
	if (MPCInstance)
	{
		FVector WorldLocation = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;

		MPCInstance->SetVectorParameterValue(
			MPCLocationParam,
			FLinearColor(WorldLocation.X, WorldLocation.Y, WorldLocation.Z));

		/*UE_LOG(LOSVision, Log,
			TEXT("UCameraVisionManager::UpdateCameraLOS >> CenterLocation=%s, VisibleRange=%f"),
			*WorldLocation.ToString(), CameraVisionRange);*/
	}
	else
	{
		/*UE_LOG(LOSVision, Warning,
			TEXT("UCameraVisionManager::UpdateCameraLOS >> PostProcessMPC instance not found!"));*/
	}

	/*UE_LOG(LOSVision, Log,
		TEXT("UCameraVisionManager::UpdateCameraLOS >> Update finished"));*/
}

//Internal helper for the Drawing LOS stamp
void UCameraVisionManager::DrawLOSStamp(UCanvas* Canvas, const TArray<ULineOfSightComponent*>& Providers,
	const FLinearColor& Color)
{
	if (!Canvas) return;// invalid canvas, just leave
	if (Providers.Num() == 0) return; // nothing to draw, early return
	
	for (ULineOfSightComponent* Provider : Providers)
	{
		if (!Provider || !Provider->GetOwner() || !Provider->GetLOSMaterialMID())// no no case!
			continue;
		
		FVector2D PixelPos;
		float TileSize;
		
		if (!ConvertWorldToRT(
			Provider->GetOwner()->GetActorLocation(),
			Provider->GetVisibleRange(),
			PixelPos, TileSize))
			continue;

		FCanvasTileItem Tile(
			PixelPos - FVector2D(TileSize*0.5f, TileSize*0.5f),
			Provider->GetLOSMaterialMID()->GetRenderProxy(),
			FVector2D(TileSize, TileSize)
		);

		Tile.BlendMode = SE_BLEND_AlphaBlend;
		Tile.SetColor(Color);
		Canvas->DrawItem(Tile);
	}
}

void UCameraVisionManager::RenderLOS_GPU(FRDGBuilder& GraphBuilder, FRDGTextureRef LOSTexture)
{
	TArray<ULineOfSightComponent*> ActiveProviders;
	if (!GetVisibleProviders(ActiveProviders))
	{
		return;
	}

	TArray<FLOSStampData> Stamps;//catcher for the stamps data
	Stamps.Reserve(ActiveProviders.Num());

	for (ULineOfSightComponent* Provider : ActiveProviders)
	{
		if (!Provider || !Provider->IsUpdating())
			continue;

		FVector2D PixelPos;
		float TileSize;

		if (!ConvertWorldToRT(
			Provider->GetOwner()->GetActorLocation(),
			Provider->GetVisibleRange(),
			PixelPos,
			TileSize))
		{
			continue;
		}

		FLOSStampData Stamp;
		Stamp.CenterRadiusStrength = FVector4f(
			PixelPos.X / CameraLocalRT->SizeX,
			PixelPos.Y / CameraLocalRT->SizeY,
			TileSize / CameraLocalRT->SizeX,
			1.0f);

		Stamp.ChannelBitMask =
			1u << static_cast<uint8>(Provider->GetVisionChannel());

		Stamps.Add(Stamp);
	}

	const uint32 ViewMask =
		1u << static_cast<uint8>(VisionChannel);

	AddLOSStampPass(
		GraphBuilder,
		LOSTexture,
		Stamps,
		ViewMask,
		/*bClearBeforeStamp=*/true);
}

void UCameraVisionManager::DrawLOS_CPU(UCanvas* Canvas, int32 Width, int32 Height)
{
	/*UE_LOG(LOSVision, Log,
		TEXT("UCameraVisionManager::DrawLOS >> Canvas=%s, Width=%d, Height=%d"),
		*GetNameSafe(Canvas), Width, Height);*/

	if (!Canvas || !CameraLocalRT)
	{
		UE_LOG(LOSVision, Warning,
			TEXT("UCameraVisionManager::DrawLOS >> Canvas or CameraLocalRT is null"));
		return;
	}
	
	//Draw the tile with 0 alpha texture first to clear completely
	FCanvasTileItem ClearTile(
		FVector2D(0,0),
		FVector2D(Width,Height),
		FLinearColor(0,0,0,0));
	
	ClearTile.BlendMode = SE_BLEND_Opaque; // Use opaque to fully overwrite
	Canvas->DrawItem(ClearTile);


	TArray<ULineOfSightComponent*> ActiveProviders;//catchers
	if (!GetVisibleProviders(ActiveProviders))
	{
		/*UE_LOG(LOSVision, Error,
			TEXT("UCameraVisionManager::DrawLOS >> Failed to bring VisibleProviders"));*/
		return;
	}
	int32 CompositedCount = 0;

	//Make Container for VisionChannel Providers, so that it can reduce unnecessary iteration
	TArray<ULineOfSightComponent*> VCShared;
	TArray<ULineOfSightComponent*> VCTeamA;
	TArray<ULineOfSightComponent*> VCTeamB;
	TArray<ULineOfSightComponent*> VCTeamC;

	for (ULineOfSightComponent* Provider : ActiveProviders)
	{
		if (!Provider || !Provider->IsUpdating() || !Provider->GetLOSMaterialMID())
			continue;

		switch (Provider->GetVisionChannel())
		{
		case EVisionChannel::SharedVision: VCShared.Add(Provider); break;
		case EVisionChannel::TeamA: VCTeamA.Add(Provider); break;
		case EVisionChannel::TeamB: VCTeamB.Add(Provider); break;
		case EVisionChannel::TeamC: VCTeamC.Add(Provider); break;
			
		case EVisionChannel::None:
		default: break;
		}
	}

	DrawLOSStamp(Canvas, VCShared, FLinearColor(1,1,1,1));
	DrawLOSStamp(Canvas,VCTeamA, FLinearColor(1,0,0,1));
	DrawLOSStamp(Canvas,VCTeamB, FLinearColor(0,1,0,1));
	DrawLOSStamp(Canvas,VCTeamC, FLinearColor(0,0,1,1));

	CompositedCount = VCShared.Num() + VCTeamA.Num() + VCTeamB.Num() + VCTeamC.Num();
	
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

	/*UE_LOG(LOSVision, Log,
		TEXT("UCameraVisionManager::DrawLOS >> Composited %d providers"),
		CompositedCount);*/
}

bool UCameraVisionManager::ConvertWorldToRT(
	const FVector& ProviderWorldLocation,
	const float& ProviderVisionRange,
	FVector2D& OutPixelPosition,
	float& OutTileSize) const
{
	if (!CameraLocalRT || CameraVisionRange <= 0.f)
		return false;

	// Offset from center
	FVector Delta = ProviderWorldLocation - GetOwner()->GetActorLocation();
	
	// Map world offset to pixel space
	float PixelX = (0.5f + (Delta.X / (2.f * CameraVisionRange))) * CameraLocalRT->SizeX;
	float PixelY = (0.5f + (Delta.Y / (2.f * CameraVisionRange))) * CameraLocalRT->SizeY;

	OutPixelPosition = FVector2D(PixelX, PixelY);

	// Tile size proportional to provider vision
	OutTileSize = (ProviderVisionRange / CameraVisionRange) * CameraLocalRT->SizeX;
	OutTileSize = FMath::Max(4.f, OutTileSize);
	
	return true;
}

bool UCameraVisionManager::GetVisibleProviders(TArray<ULineOfSightComponent*>& OutProviders) const
{
	if (VisionChannel==EVisionChannel::None)
	{
		/*UE_LOG(LOSVision, Error,
			TEXT("UCameraVisionManager::GetVisibleProviders >> Invalid VisionChannel"));*/
		return false;
	}
	ULOSVisionSubsystem* Subsystem = GetWorld()->GetSubsystem<ULOSVisionSubsystem>();
	if (!Subsystem)
	{
		/*UE_LOG(LOSVision, Error,
			TEXT("UCameraVisionManager::GetVisibleProviders >> VisionSubsystem not found"));*/
		return false;
	}
	//Get the Teams
	OutProviders = Subsystem->GetProvidersForTeam(VisionChannel);
	return true;
}

bool UCameraVisionManager::ShouldRunClientLogic() const
{
	if (GetNetMode() == NM_DedicatedServer)
		return false;
	
	// other conditions



	return true;
}

uint32 UCameraVisionManager::MakeChannelBitMask(const TArray<EVisionChannel>& ChannelEnums)
{
	uint32 Mask = 0;

	for (EVisionChannel Channel : ChannelEnums)
	{
		if (Channel == EVisionChannel::None)
		{
			continue;
		}

		uint8 Index = static_cast<uint8>(Channel);
		check(Index < 32);

		Mask |= (1u << Index);
	}

	return Mask;
}




