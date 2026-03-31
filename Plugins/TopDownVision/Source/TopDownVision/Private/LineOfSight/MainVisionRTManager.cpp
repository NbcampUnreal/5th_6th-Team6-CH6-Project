#include "LineOfSight/MainVisionRTManager.h"

#include "RenderGraphUtils.h"
#include "Engine/World.h"
#include "Engine/Canvas.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "GameFramework/GameStateBase.h"

#include "TopDownVision/Public/LineOfSight/VisionComps/Vision_VisualComp.h"
#include "LineOfSight/GPU/LOSStampPass.h"
#include "LineOfSight/Management/VisionGameStateComp.h"
#include "LineOfSight/Management/Subsystem/LOSVisionSubsystem.h"
#include "LineOfSight/Management/Subsystem/LOSRequirementPoolSubsystem.h"
#include "LineOfSight/Management/VisionPlayerStateComp.h"
#include "TopDownVisionDebug.h"


UMainVisionRTManager::UMainVisionRTManager()
{
    PrimaryComponentTick.bCanEverTick = false;

    UE_LOG(LOSVision, Log,
        TEXT("UMainVisionRTManager::Constructor >> Component constructed"));
}

void UMainVisionRTManager::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LOSVision, Log, TEXT("UMainVisionRTManager::BeginPlay >> BeginPlay called"));
}

// -------------------------------------------------------------------------- //
//  Initialize
// -------------------------------------------------------------------------- //

void UMainVisionRTManager::InitializeMainVisionRTComp()
{
    if (!ShouldRunClientLogic())
        return;

    if (!CameraLocalRT)
    {
        UE_LOG(LOSVision, Warning,
            TEXT("UMainVisionRTManager::InitializeMainVisionRTComp >> CameraLocalRT is null."));
        return;
    }

    if (bUseCPU)
    {
        CameraLocalRT->OnCanvasRenderTargetUpdate.AddDynamic(
            this, &UMainVisionRTManager::DrawLOS_CPU);
        CameraLocalRT->UpdateResource();

        UE_LOG(LOSVision, Log,
            TEXT("UMainVisionRTManager::InitializeMainVisionRTComp >> Bound DrawLOS_CPU to: %s"),
            *CameraLocalRT->GetName());
    }

    MPCInstance = GetWorld()->GetParameterCollectionInstance(PostProcessMPC);
    if (MPCInstance)
    {
        FVector WorldLocation = GetOwner()
            ? GetOwner()->GetActorLocation()
            : FVector::ZeroVector;

        MPCInstance->SetVectorParameterValue(MPCLocationParam,
            FLinearColor(WorldLocation.X, WorldLocation.Y, WorldLocation.Z));
        MPCInstance->SetScalarParameterValue(MPCVisibleRangeParam, CameraVisionRange);
    }

    if (LayeredLOSInterfaceMaterial)
    {
        LayeredLOSInterfaceMID = UMaterialInstanceDynamic::Create(
            LayeredLOSInterfaceMaterial, this);
        if (LayeredLOSInterfaceMID)
        {
            UTexture* BindTarget = FeatheredRT
                ? (UTexture*)FeatheredRT
                : (UTexture*)CameraLocalRT;
            LayeredLOSInterfaceMID->SetTextureParameterValue(
                LayeredLOSTextureParam, BindTarget);
        }
    }

    if (!FeatherMID && FeatherOutMaterial)
    {
        FeatherMID = UMaterialInstanceDynamic::Create(FeatherOutMaterial, this);
        if (FeatherMID)
        {
            FeatherMID->SetTextureParameterValue(TEXT("InputTexture"), CameraLocalRT);
        }
        else
        {
            UE_LOG(LOSVision, Error,
                TEXT("UMainVisionRTManager::InitializeMainVisionRTComp >> Failed to create FeatherMID"));
        }
    }
    else if (!FeatherOutMaterial)
    {
        UE_LOG(LOSVision, Warning,
            TEXT("UMainVisionRTManager::InitializeMainVisionRTComp >> FeatherOutMaterial not assigned"));
    }

    Activate();
}

// -------------------------------------------------------------------------- //

static bool RectOverlapsWorld(
    const FVector& ACenter, float AHalfSize,
    const FVector& BCenter, float BHalfSize)
{
    return !(
        FMath::Abs(ACenter.X - BCenter.X) > (AHalfSize + BHalfSize) ||
        FMath::Abs(ACenter.Y - BCenter.Y) > (AHalfSize + BHalfSize)
    );
}

// -------------------------------------------------------------------------- //
//  Update
// -------------------------------------------------------------------------- //

void UMainVisionRTManager::DrawLOSStampsBatched(
    UTextureRenderTarget2D* TargetRT,
    const TArray<UVision_VisualComp*>& Providers,
    const FLinearColor& Color)
{
    if (!TargetRT || Providers.Num() == 0)
        return;

    UCanvas* Canvas = nullptr;
    FDrawToRenderTargetContext Context;
    FVector2D RTSize(TargetRT->SizeX, TargetRT->SizeY);

    UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(
        GetWorld(), TargetRT, Canvas, RTSize, Context);
    if (!Canvas)
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

        const float Alpha = Provider->GetVisibilityAlpha();
        if (Alpha <= KINDA_SMALL_NUMBER)
            continue;

        FCanvasTileItem Tile(
            PixelPos - FVector2D(TileSize * 0.5f, TileSize * 0.5f),
            Provider->GetStampMID()->GetRenderProxy(),
            FVector2D(TileSize, TileSize)
        );
        Tile.BlendMode = SE_BLEND_AlphaBlend;
        Tile.SetColor(FLinearColor(
            Color.R * Alpha, Color.G * Alpha,
            Color.B * Alpha, Color.A * Alpha));
        Canvas->DrawItem(Tile);
    }

    UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), Context);
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
        return;

    const FVector CameraCenter = GetOwner()->GetActorLocation();

    CachedValidProviders.Reset();

    for (UVision_VisualComp* Provider : ActiveProviders)
    {
        if (!Provider || !Provider->GetOwner())
            continue;

        const bool bInRange = RectOverlapsWorld(
            CameraCenter,                             CameraVisionRange,
            Provider->GetOwner()->GetActorLocation(), Provider->GetVisibleRange());

        Provider->ToggleLOSStampUpdate(bInRange);

        if (bInRange)
        {
            // Pool mode: skip if no slot assigned.
            // Slot is acquired/released via OnRevealed/OnHidden on Vision_VisualComp.
            if (Provider->UsesResourcePool() && !Provider->HasPoolSlot())
                continue;

            // UpdateVision runs the obstacle drawer and stamp update.
            // For remote team providers the obstacle RT is null (InitializeSamplerOnly
            // only), so obstacle drawing is skipped harmlessly — their stamp
            // renders as a full circle with no occlusion, which is the correct
            // approximation for a teammate's vision on your screen.
            // The local player's vision retains full wall occlusion.
            Provider->UpdateVision();

            if (Provider->IsUpdating() && Provider->GetStampMID())
                CachedValidProviders.Add(Provider);
        }
    }

    if (bUseCPU)
    {
        CameraLocalRT->UpdateResource();
        ApplyFeatheredBlurToRT();
    }
    else
    {
        TArray<FLOSStampData> StampData_GT;
        StampData_GT.Reserve(CachedValidProviders.Num());

        for (UVision_VisualComp* Provider : CachedValidProviders)
        {
            AActor* Owner = Provider->GetOwner();
            if (!Owner)
                continue;

            const FVector WorldPos = Owner->GetActorLocation();
            const FVector2f CenterUV(
                (WorldPos.X - CameraCenter.X) / (CameraVisionRange * 2.f) + 0.5f,
                (WorldPos.Y - CameraCenter.Y) / (CameraVisionRange * 2.f) + 0.5f
            );
            const float RadiusUV =
                Provider->GetVisibleRange() / (CameraVisionRange * 2.f);

            const EVisionChannel V_Channel = Provider->GetVisionChannel();
            const uint32 ChannelBitMask = [V_Channel]() -> uint32
            {
                if (V_Channel == EVisionChannel::None)          return 0u;
                if (V_Channel == EVisionChannel::AlwaysVisible) return 0xFFFFFFFFu;
                return 1u << static_cast<uint32>(V_Channel);
            }();

            FLOSStampData& Stamp = StampData_GT.AddDefaulted_GetRef();
            Stamp.CenterRadiusStrength = FVector4f(
                CenterUV.X, CenterUV.Y, RadiusUV,
                Provider->GetVisibilityAlpha());
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
        FVector WorldLocation = GetOwner()
            ? GetOwner()->GetActorLocation()
            : FVector::ZeroVector;
        MPCInstance->SetVectorParameterValue(MPCLocationParam,
            FLinearColor(WorldLocation.X, WorldLocation.Y, WorldLocation.Z));
    }
}

// -------------------------------------------------------------------------- //
//  Draw — CPU path
// -------------------------------------------------------------------------- //

void UMainVisionRTManager::DrawLOS_CPU(UCanvas* Canvas, int32 Width, int32 Height)
{
    if (!Canvas || !CameraLocalRT)
    {
        UE_LOG(LOSVision, Warning,
            TEXT("UMainVisionRTManager::DrawLOS_CPU >> Canvas or CameraLocalRT is null"));
        return;
    }

    FCanvasTileItem ClearTile(
        FVector2D(0, 0), FVector2D(Width, Height),
        FLinearColor(0, 0, 0, 0));
    ClearTile.BlendMode = SE_BLEND_Opaque;
    Canvas->DrawItem(ClearTile);

    if (CachedValidProviders.IsEmpty())
        return;

    TArray<UVision_VisualComp*> RawProviders;
    RawProviders.Reserve(CachedValidProviders.Num());
    for (TObjectPtr<UVision_VisualComp>& P : CachedValidProviders)
        if (P) RawProviders.Add(P);

    DrawLOSStamp(Canvas, RawProviders, FLinearColor(1, 1, 1, 1));

    if (bDrawTextureRange)
    {
        DrawDebugBox(GetWorld(), GetOwner()->GetActorLocation(),
            FVector(CameraVisionRange, CameraVisionRange, 50.f),
            FQuat::Identity, FColor::Green, false, -1.f, 0, 2.f);
    }
}

void UMainVisionRTManager::DrawLOSStamp(
    UCanvas* Canvas,
    const TArray<UVision_VisualComp*>& Providers,
    const FLinearColor& Color)
{
    if (!Canvas || Providers.Num() == 0)
        return;

    for (UVision_VisualComp* Provider : Providers)
    {
        if (!Provider) continue;

        UMaterialInstanceDynamic* MID = Provider->GetStampMID();
        if (!MID) continue;

        FVector2D PixelPos;
        float TileSize;

        if (!ConvertWorldToRT(
            Provider->GetOwner()->GetActorLocation(),
            Provider->GetVisibleRange(),
            PixelPos, TileSize))
            continue;

        const float Alpha = Provider->GetVisibilityAlpha();
        if (Alpha <= KINDA_SMALL_NUMBER)
            continue;

        FCanvasTileItem Tile(
            PixelPos - FVector2D(TileSize * 0.5f, TileSize * 0.5f),
            MID->GetRenderProxy(),
            FVector2D(TileSize, TileSize)
        );
        Tile.BlendMode = SE_BLEND_AlphaBlend;
        Tile.SetColor(FLinearColor(
            Color.R * Alpha, Color.G * Alpha,
            Color.B * Alpha, Color.A * Alpha));
        Canvas->DrawItem(Tile);
    }
}

// -------------------------------------------------------------------------- //
//  Draw — GPU path (stub)
// -------------------------------------------------------------------------- //

void UMainVisionRTManager::RenderLOS_GPU(
    FRDGBuilder& GraphBuilder, FRDGTextureRef LOSTexture)
{
}

// -------------------------------------------------------------------------- //
//  Helpers
// -------------------------------------------------------------------------- //

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

bool UMainVisionRTManager::GetVisibleProviders(
    TArray<UVision_VisualComp*>& OutProviders) const
{
    ULOSVisionSubsystem* Subsystem = GetWorld()->GetSubsystem<ULOSVisionSubsystem>();
    if (!Subsystem)
    {
        UE_LOG(LOSVision, Warning,
            TEXT("UMainVisionRTManager::GetVisibleProviders >> Subsystem not found"));
        return false;
    }

    UVisionPlayerStateComp* LocalVisionPS =
        ULOSVisionSubsystem::GetLocalVisionPS(GetWorld());

    if (LocalVisionPS && LocalVisionPS->GetTeamChannel() != EVisionChannel::None)
    {
        // Use CanSeeTeam across all providers — identical filter to what
        // InitializeSameTeamEvaluators and ReevaluateTargetVisibility use.
        // This ensures the RT stamps exactly the providers whose evaluators
        // are running locally.
        for (UVision_VisualComp* P : Subsystem->GetAllProviders())
        {
            if (!P || !P->GetOwner())
                continue;

            if (LocalVisionPS->CanSeeTeam(P->GetVisionChannel()))
                OutProviders.AddUnique(P);
        }
    }

    // Fallback — team not yet replicated, use the local pawn's own provider
    // so the screen is never blank during early frames.
    if (OutProviders.IsEmpty())
    {
        APlayerController* PC = GEngine->GetFirstLocalPlayerController(GetWorld());
        if (APawn* Pawn = PC ? PC->GetPawn() : nullptr)
            if (UVision_VisualComp* Own =
                Pawn->FindComponentByClass<UVision_VisualComp>())
                OutProviders.Add(Own);
    }

    return OutProviders.Num() > 0;
}

bool UMainVisionRTManager::ShouldRunClientLogic() const
{
    return GetNetMode() != NM_DedicatedServer;
}

ULOSRequirementPoolSubsystem* UMainVisionRTManager::GetPoolSubsystem() const
{
    if (!CachedPoolSubsystem)
        CachedPoolSubsystem = GetWorld()->GetSubsystem<ULOSRequirementPoolSubsystem>();

    return CachedPoolSubsystem;
}

void UMainVisionRTManager::ApplyFeatheredBlurToRT()
{
    if (!FeatherMID || !FeatheredRT)
        return;

    FeatherMID->SetTextureParameterValue(TEXT("InputTexture"), CameraLocalRT);

    UCanvas* Canvas = nullptr;
    FDrawToRenderTargetContext Context;
    FVector2D RTSize(FeatheredRT->SizeX, FeatheredRT->SizeY);

    UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(
        GetWorld(), FeatheredRT, Canvas, RTSize, Context);

    if (Canvas)
    {
        FCanvasTileItem Tile(
            FVector2D(0, 0),
            FeatherMID->GetRenderProxy(),
            FVector2D(FeatheredRT->SizeX, FeatheredRT->SizeY)
        );
        Tile.BlendMode = SE_BLEND_Opaque;
        Canvas->DrawItem(Tile);

        UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), Context);
    }
}

uint32 UMainVisionRTManager::MakeChannelBitMask(
    const TArray<EVisionChannel>& ChannelEnums)
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