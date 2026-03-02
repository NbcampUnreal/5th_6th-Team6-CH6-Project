#include "LineOfSight/MainVisionRTManager.h"

#include "RenderGraphUtils.h"
#include "TopDownVision/Public/LineOfSight/VisionComps/Vision_VisualComp.h"
#include "Engine/World.h"
#include "Engine/Canvas.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "TopDownVisionDebug.h"
#include "GameFramework/GameStateBase.h"
#include "LineOfSight/GPU/LOSStampPass.h"
#include "LineOfSight/Management/VisionGameStateComp.h"
#include "LineOfSight/Management/Subsystem/LOSVisionSubsystem.h"


UMainVisionRTManager::UMainVisionRTManager()
{
	UE_LOG(LOSVision, Log,
		TEXT("UMainVisionRTManager::Constructor >> Component constructed"));
}

void UMainVisionRTManager::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LOSVision, Log, TEXT("UMainVisionRTManager::BeginPlay >> BeginPlay called"));
}

void UMainVisionRTManager::Initialize()
{
	if (!ShouldRunClientLogic())
		return;

	if (!CameraLocalRT)
	{
		UE_LOG(LOSVision, Warning,
			TEXT("UMainVisionRTManager::Initialize >> CameraLocalRT is null. Assign it in the Content Browser."));
		return;
	}
	
	if (!TempBlurRT)
	{
		TempBlurRT = UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(
			GetWorld(),
			UCanvasRenderTarget2D::StaticClass(),
			CameraLocalRT->SizeX,
			CameraLocalRT->SizeY
		);
		TempBlurRT->ClearColor = FLinearColor::Black;
		TempBlurRT->UpdateResourceImmediate();
	}

	if (bUseCPU)
	{
		CameraLocalRT->OnCanvasRenderTargetUpdate.AddDynamic(this, &UMainVisionRTManager::DrawLOS_CPU);
		CameraLocalRT->UpdateResource();

		UE_LOG(LOSVision, Log, TEXT("UMainVisionRTManager::Initialize >> Initialized with CameraLocalRT: %s"),
			*CameraLocalRT->GetName());
	}

	MPCInstance = GetWorld()->GetParameterCollectionInstance(PostProcessMPC);
	if (MPCInstance)
	{
		FVector WorldLocation = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
		MPCInstance->SetVectorParameterValue(MPCLocationParam,
			FLinearColor(WorldLocation.X, WorldLocation.Y, WorldLocation.Z));
		MPCInstance->SetScalarParameterValue(MPCVisibleRangeParam, CameraVisionRange);
	}

	if (LayeredLOSInterfaceMaterial)
	{
		LayeredLOSInterfaceMID = UMaterialInstanceDynamic::Create(LayeredLOSInterfaceMaterial, this);
		if (LayeredLOSInterfaceMID)
		{
			LayeredLOSInterfaceMID->SetTextureParameterValue(LayeredLOSTextureParam, CameraLocalRT);
		}
	}

	if (!FeatherMID && FeatherOutMaterial)
	{
		FeatherMID = UMaterialInstanceDynamic::Create(FeatherOutMaterial, this);
		if (!FeatherMID)
		{
			UE_LOG(LOSVision, Error, TEXT("UMainVisionRTManager::Initialize >> Failed to create FeatherMID"));
		}
		else if (CameraLocalRT)
		{
			FeatherMID->SetTextureParameterValue(TEXT("InputTexture"), CameraLocalRT);
		}
		else
		{
			UE_LOG(LOSVision, Warning,
				TEXT("UMainVisionRTManager::Initialize >> CameraLocalRT is null, cannot bind to FeatherMID"));
		}
	}
	else if (!FeatherOutMaterial)
	{
		UE_LOG(LOSVision, Warning, TEXT("UMainVisionRTManager::Initialize >> FeatherMaterial is not assigned!"));
	}
}

// Internal helper
static bool RectOverlapsWorld(
	const FVector& ACenter, float AHalfSize,
	const FVector& BCenter, float BHalfSize)
{
	return !(
		FMath::Abs(ACenter.X - BCenter.X) > (AHalfSize + BHalfSize) ||
		FMath::Abs(ACenter.Y - BCenter.Y) > (AHalfSize + BHalfSize)
	);
}

void UMainVisionRTManager::UpdateCameraLOS()
{
	if (!ShouldRunClientLogic())
        return;

    if (!CameraLocalRT)
    {
        UE_LOG(LOSVision, Error,
            TEXT("UMainVisionRTManager::UpdateCameraLOS >> CameraLocalRT is null!"));
        return;
    }

    TArray<UVision_VisualComp*> ActiveProviders;
    if (!GetVisibleProviders(ActiveProviders))
    {
        UE_LOG(LOSVision, Error,
            TEXT("UMainVisionRTManager::UpdateCameraLOS >> Failed to bring VisibleProviders"));
        return;
    }

    // Cache VisionGameStateComp for visibility filtering
    UVisionGameStateComp* GSComp = nullptr;
    if (AGameStateBase* GS = GetWorld()->GetGameState())
        GSComp = GS->FindComponentByClass<UVisionGameStateComp>();

    const FVector CameraCenter = GetOwner()->GetActorLocation();

    for (UVision_VisualComp* Provider : ActiveProviders)
    {
        if (!Provider || !Provider->GetOwner())
            continue;

        const bool bInRange = RectOverlapsWorld(
            CameraCenter,
            CameraVisionRange,
            Provider->GetOwner()->GetActorLocation(),
            Provider->GetVisibleRange());

        // Skip LOS update if provider's owner is hidden
        const bool bProviderVisible = GSComp
            ? GSComp->IsActorVisibleToTeam(Provider->GetOwner(), VisionChannel)
            : true;

        Provider->ToggleLOSStampUpdate(bInRange && bProviderVisible);

        if (bInRange && bProviderVisible)
            Provider->UpdateVision();
    }

    if (bUseCPU)
    {
        CameraLocalRT->UpdateResource();
        ApplyFeatheredBlurToRT();
    }
    else
    {
        TArray<FLOSStampData> StampData_GT;
        StampData_GT.Reserve(ActiveProviders.Num());

        for (UVision_VisualComp* Provider : ActiveProviders)
        {
            if (!Provider || !Provider->IsUpdating())
                continue;

            AActor* Owner = Provider->GetOwner();
            if (!Owner)
                continue;

            const FVector WorldPos = Owner->GetActorLocation();
            const float MaxRange   = CameraVisionRange;

            const FVector2f CenterUV(
                (WorldPos.X - CameraCenter.X) / (MaxRange * 2.f) + 0.5f,
                (WorldPos.Y - CameraCenter.Y) / (MaxRange * 2.f) + 0.5f
            );

            const float RadiusUV = Provider->GetVisibleRange() / (MaxRange * 2.f);

            const EVisionChannel V_Channel = Provider->GetVisionChannel();
            const uint32 ChannelBitMask =
                (V_Channel == EVisionChannel::None)
                    ? 0u
                    : (1u << static_cast<uint32>(V_Channel));

            FLOSStampData& Stamp = StampData_GT.AddDefaulted_GetRef();
            Stamp.CenterRadiusStrength = FVector4f(CenterUV.X, CenterUV.Y, RadiusUV, Provider->GetVisibilityAlpha());
            Stamp.ChannelBitMask = ChannelBitMask;
        }

        const TArray<FLOSStampData> StampData_RT = StampData_GT;
        const uint32 ViewMask = CameraViewChannelMask;

        ENQUEUE_RENDER_COMMAND(UpdateLOS_GPU)(
            [this, StampData_RT, ViewMask](FRHICommandListImmediate& RHICmdList)
            {
                FRDGBuilder GraphBuilder(RHICmdList);
                FRDGTextureRef LOSTexture = RegisterExternalTexture(
                    GraphBuilder,
                    CameraLocalRT->GetRenderTargetResource()->GetRenderTargetTexture(),
                    TEXT("CameraLOS_GPU"));
                AddLOSStampPass(GraphBuilder, LOSTexture, StampData_RT, ViewMask, true);
                GraphBuilder.Execute();
            });
    }

    if (MPCInstance)
    {
        FVector WorldLocation = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
        MPCInstance->SetVectorParameterValue(MPCLocationParam,
            FLinearColor(WorldLocation.X, WorldLocation.Y, WorldLocation.Z));
    }
}

void UMainVisionRTManager::DrawLOSStamp(UCanvas* Canvas,
	const TArray<UVision_VisualComp*>& Providers,
	const FLinearColor& Color)
{
	if (!Canvas || Providers.Num() == 0)
		return;

	for (UVision_VisualComp* Provider : Providers)
	{
		if (!Provider || !Provider->GetOwner() || !Provider->GetStampMID())
			continue;

		FVector2D PixelPos;
		float TileSize;

		if (!ConvertWorldToRT(
			Provider->GetOwner()->GetActorLocation(),
			Provider->GetVisibleRange(),
			PixelPos, TileSize))
			continue;

		// Use VisibilityAlpha for fade — stamp fades in/out with the unit
		const float Alpha = Provider->GetVisibilityAlpha();
		if (Alpha <= KINDA_SMALL_NUMBER)
			continue; // fully hidden — skip draw entirely

		FCanvasTileItem Tile(
			PixelPos - FVector2D(TileSize * 0.5f, TileSize * 0.5f),
			Provider->GetStampMID()->GetRenderProxy(),
			FVector2D(TileSize, TileSize)
		);
		Tile.BlendMode = SE_BLEND_AlphaBlend;
		Tile.SetColor(FLinearColor(Color.R, Color.G, Color.B, Color.A * Alpha));
		Canvas->DrawItem(Tile);
	}
}

void UMainVisionRTManager::RenderLOS_GPU(FRDGBuilder& GraphBuilder, FRDGTextureRef LOSTexture)
{
	TArray<UVision_VisualComp*> ActiveProviders;
	if (!GetVisibleProviders(ActiveProviders))
		return;

	TArray<FLOSStampData> Stamps;
	Stamps.Reserve(ActiveProviders.Num());

	for (UVision_VisualComp* Provider : ActiveProviders)
	{
		if (!Provider || !Provider->IsUpdating())
			continue;

		FVector2D PixelPos;
		float TileSize;

		if (!ConvertWorldToRT(
			Provider->GetOwner()->GetActorLocation(),
			Provider->GetVisibleRange(),
			PixelPos, TileSize))
			continue;

		FLOSStampData Stamp;
		Stamp.CenterRadiusStrength = FVector4f(
			PixelPos.X / CameraLocalRT->SizeX,
			PixelPos.Y / CameraLocalRT->SizeY,
			TileSize   / CameraLocalRT->SizeX,
			1.0f);
		Stamp.ChannelBitMask = 1u << static_cast<uint8>(Provider->GetVisionChannel());
		Stamps.Add(Stamp);
	}

	AddLOSStampPass(GraphBuilder, LOSTexture, Stamps,
		1u << static_cast<uint8>(VisionChannel), true);
}

void UMainVisionRTManager::DrawLOS_CPU(UCanvas* Canvas, int32 Width, int32 Height)
{
	if (!Canvas || !CameraLocalRT)
	{
		UE_LOG(LOSVision, Warning,
			TEXT("UMainVisionRTManager::DrawLOS_CPU >> Canvas or CameraLocalRT is null"));
		return;
	}
	
	FCanvasTileItem ClearTile(FVector2D(0, 0), FVector2D(Width, Height), FLinearColor(0, 0, 0, 0));
	ClearTile.BlendMode = SE_BLEND_Opaque;
	Canvas->DrawItem(ClearTile);

	TArray<UVision_VisualComp*> ActiveProviders;
	if (!GetVisibleProviders(ActiveProviders))
		return;

	int32 CompositedCount = 0;

	TArray<UVision_VisualComp*> VCShared;
	TArray<UVision_VisualComp*> VCTeamA;
	TArray<UVision_VisualComp*> VCTeamB;
	TArray<UVision_VisualComp*> VCTeamC;

	for (UVision_VisualComp* Provider : ActiveProviders)
	{
		if (!Provider || !Provider->IsUpdating() || !Provider->GetStampMID())
			continue;

		switch (Provider->GetVisionChannel())
		{
		case EVisionChannel::SharedVision: VCShared.Add(Provider); break;
		case EVisionChannel::TeamA:        VCTeamA.Add(Provider);  break;
		case EVisionChannel::TeamB:        VCTeamB.Add(Provider);  break;
		case EVisionChannel::TeamC:        VCTeamC.Add(Provider);  break;
		case EVisionChannel::None:
		default: break;
		}
	}

	DrawLOSStamp(Canvas, VCShared, FLinearColor(1, 1, 1, 1));
	DrawLOSStamp(Canvas, VCTeamA,  FLinearColor(1, 0, 0, 1));
	DrawLOSStamp(Canvas, VCTeamB,  FLinearColor(0, 1, 0, 1));
	DrawLOSStamp(Canvas, VCTeamC,  FLinearColor(0, 0, 1, 1));

	CompositedCount = VCShared.Num() + VCTeamA.Num() + VCTeamB.Num() + VCTeamC.Num();

	if (bDrawTextureRange)
	{
		DrawDebugBox(GetWorld(), GetOwner()->GetActorLocation(),
			FVector(CameraVisionRange, CameraVisionRange, 50.f),
			FQuat::Identity, FColor::Green, false, -1.f, 0, 2.f);
	}
}

bool UMainVisionRTManager::ConvertWorldToRT(
	const FVector& ProviderWorldLocation,
	const float& ProviderVisionRange,
	FVector2D& OutPixelPosition,
	float& OutTileSize) const
{
	if (!CameraLocalRT || CameraVisionRange <= 0.f)
		return false;

	FVector Delta = ProviderWorldLocation - GetOwner()->GetActorLocation();
	
	float PixelX = (0.5f + (Delta.X / (2.f * CameraVisionRange))) * CameraLocalRT->SizeX;
	float PixelY = (0.5f + (Delta.Y / (2.f * CameraVisionRange))) * CameraLocalRT->SizeY;
	OutPixelPosition = FVector2D(PixelX, PixelY);

	OutTileSize = (ProviderVisionRange / CameraVisionRange) * CameraLocalRT->SizeX;
	OutTileSize = FMath::Max(4.f, OutTileSize);
	
	return true;
}

bool UMainVisionRTManager::GetVisibleProviders(TArray<UVision_VisualComp*>& OutProviders) const
{
	if (VisionChannel == EVisionChannel::None)
		return false;

	ULOSVisionSubsystem* Subsystem = GetWorld()->GetSubsystem<ULOSVisionSubsystem>();
	if (!Subsystem)
		return false;

	OutProviders = Subsystem->GetProvidersForTeam(VisionChannel);
	return true;
}

bool UMainVisionRTManager::ShouldRunClientLogic() const
{
	if (GetNetMode() == NM_DedicatedServer)
		return false;

	return true;
}

void UMainVisionRTManager::ApplyFeatheredBlurToRT()
{
	if (!CameraLocalRT || !TempBlurRT || !FeatherMID)
		return;

	{
		UCanvas* Canvas = nullptr;
		FDrawToRenderTargetContext Context;
		FVector2D TempRTSize(TempBlurRT->SizeX, TempBlurRT->SizeY);

		UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(
			GetWorld(), TempBlurRT, Canvas, TempRTSize, Context);

		if (Canvas)
		{
			DrawLOS_CPU(Canvas, TempBlurRT->SizeX, TempBlurRT->SizeY);
			UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), Context);
		}
	}

	{
		UCanvas* Canvas = nullptr;
		FDrawToRenderTargetContext Context;
		FVector2D FinalRTSize(CameraLocalRT->SizeX, CameraLocalRT->SizeY);

		UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(
			GetWorld(), CameraLocalRT, Canvas, FinalRTSize, Context);

		if (Canvas)
		{
			FeatherMID->SetTextureParameterValue(TEXT("InputTexture"), TempBlurRT);

			FCanvasTileItem Tile(
				FVector2D(0, 0),
				FeatherMID->GetRenderProxy(),
				FVector2D(CameraLocalRT->SizeX, CameraLocalRT->SizeY)
			);
			Tile.BlendMode = SE_BLEND_Opaque;
			Canvas->DrawItem(Tile);

			UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), Context);
		}
	}
}

uint32 UMainVisionRTManager::MakeChannelBitMask(const TArray<EVisionChannel>& ChannelEnums)
{
	uint32 Mask = 0;
	for (EVisionChannel Channel : ChannelEnums)
	{
		if (Channel == EVisionChannel::None)
			continue;

		uint8 Index = static_cast<uint8>(Channel);
		check(Index < 32);
		Mask |= (1u << Index);
	}
	return Mask;
}

