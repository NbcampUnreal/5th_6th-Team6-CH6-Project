#include "ObstacleOcclusion/MaterialOcclusion/MainOcclusionPainter.h"
#include "ObstacleOcclusion/Manager/OcclusionSubsystem.h"
#include "ObstacleOcclusion/MaterialOcclusion/OcclusionBrushTarget.h"
#include "ObstacleOcclusion/PhysicalOcclusion/FrustumToProjectionMatcherHelper.h"
#include "Engine/Engine.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TopDownVisionDebug.h"

DEFINE_LOG_CATEGORY(OcclusionPainter);

UMainOcclusionPainter::UMainOcclusionPainter()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UMainOcclusionPainter::BeginPlay()
{
    Super::BeginPlay();
}

void UMainOcclusionPainter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void UMainOcclusionPainter::InitializeOcclusionComponent(APlayerController* InPC)
{
    if (GetNetMode() == NM_DedicatedServer)
    {
        Deactivate();
        return;
    }

    if (!InPC)
    {
        UE_LOG(OcclusionPainter, Error,
            TEXT("UMainOcclusionPainter::InitializeOcclusionComponent>> Invalid PlayerController"));
        return;
    }

    // Set player controller
    SetPlayerController(InPC);

    if (!DefaultBrushMaterial)
    {
        UE_LOG(OcclusionPainter, Warning,
            TEXT("UMainOcclusionPainter::InitializeOcclusionComponent>> No default BrushMaterial set on %s — targets must supply their own"),
            *GetOwner()->GetName());
    }

    // Activate component
    Activate();
}

// ── Camera source ─────────────────────────────────────────────────────────────

void UMainOcclusionPainter::SetPlayerController(APlayerController* InPC)
{
    if (!ensureMsgf(IsValid(InPC), TEXT("UMainOcclusionPainter::SetPlayerController: null")))
        return;

    PlayerController = InPC;
    bReady = true;

    UE_LOG(OcclusionPainter, Log,
        TEXT("UMainOcclusionPainter::SetPlayerController>> Ready on %s"),
        *GetOwner()->GetName());
}

// ── Update ────────────────────────────────────────────────────────────────────

void UMainOcclusionPainter::UpdateOcclusionRT()
{
    if (!ShouldRunClientLogic()) return;
    
    if (!bReady) return;

    
    if (!IsActive()) return;
    if (!IsValid(OcclusionRT)) return;
    if (!RefreshFrustumParams()) return;

    // Clear RT to black each update
    UKismetRenderingLibrary::ClearRenderTarget2D(
        this, OcclusionRT, FLinearColor::Black);

    DrawProviderArea();
}

void UMainOcclusionPainter::DrawProviderArea()
{
    if (!IsValid(PlayerController)) return;

    UOcclusionSubsystem* Sub = GetWorld()->GetSubsystem<UOcclusionSubsystem>();
    if (!Sub) return;

    TArray<FOcclusionBrushTarget>& Targets = Sub->GetTargetsMutable();
    if (Targets.IsEmpty()) return;

    UCanvas* Canvas = nullptr;
    FVector2D CanvasSize;
    FDrawToRenderTargetContext Context;

    UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(
        this, OcclusionRT, Canvas, CanvasSize, Context);

    if (!Canvas)
    {
        UE_LOG(OcclusionPainter, Warning,
            TEXT("UMainOcclusionPainter::DrawProviderArea>> Failed to get Canvas"));
        return;
    }

    const float RTWidth  = CanvasSize.X;
    const float RTHeight = CanvasSize.Y;

    int32 VPX, VPY;
    PlayerController->GetViewportSize(VPX, VPY);

    if (VPX <= 0 || VPY <= 0)
    {
        UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, Context);
        return;
    }

    // Precompute aspect ratios once outside the loop
    const float ScreenAspect = static_cast<float>(VPX) / static_cast<float>(VPY);
    const float RTAspect     = RTWidth / RTHeight;
    const float AspectScale  = ScreenAspect / RTAspect;

    for (FOcclusionBrushTarget& Target : Targets)
    {
        if (!Target.IsValid()) continue;

        // Lazy MID init — uses target material if set, falls back to painter default
        if (!Target.IsMIDReady())
        {
            Target.InitializeMID(this, DefaultBrushMaterial);
        }

        if (!Target.IsMIDReady()) continue;

        const FVector WorldPos    = Target.PrimitiveComp->GetComponentLocation();
        const float VisibleRadius = Target.GetRadius();

        // Project world position to screen
        FVector2D ScreenPos;
        if (!PlayerController->ProjectWorldLocationToScreen(WorldPos, ScreenPos))
            continue;

        // Normalize to 0..1 viewport UV
        const FVector2D ScreenUV = ScreenPos / FVector2D(VPX, VPY);

        const float TargetDistance = FVector::Dist(
            FrustumParams.CameraLocation, WorldPos);

        if (TargetDistance <= KINDA_SMALL_NUMBER) continue;

        // Project world radius to normalized screen space (0..1 of screen height)
        const float NormalizedRadius = VisibleRadius /
            (FMath::Tan(FrustumParams.FrustumHalfAngleRad) * TargetDistance);

        // Y size — fraction of RT height
        const float BrushSizeRT_Y = NormalizedRadius * RTHeight;

        // X size — divide by AspectScale to correct screen vs RT aspect ratio
        const float BrushSizeRT_X = BrushSizeRT_Y / AspectScale;

        if (BrushSizeRT_Y <= 0.f) continue;

        // Center brush on projected screen position in RT space
        const FVector2D BrushPos(
            ScreenUV.X * RTWidth  - BrushSizeRT_X * 0.5f,
            ScreenUV.Y * RTHeight - BrushSizeRT_Y * 0.5f);

        // Only param needed — shape driven by texture, coord/size handled by tile item
        Target.BrushMID->SetScalarParameterValue(RevealAlphaParam, Target.RevealAlpha);

        const FMaterialRenderProxy* RenderProxy = Target.BrushMID->GetRenderProxy();
        if (!RenderProxy) continue;
        
        FCanvasTileItem TileItem(
            BrushPos,
            GWhiteTexture,
            FVector2D(BrushSizeRT_X, BrushSizeRT_Y),
            FLinearColor::White);

        // UV0/UV1 correctly passes 0-1 to texture sampling in material
        // unlike TexCoord[0] which returns absolute RT pixel coords
        TileItem.UV0 = FVector2D(0.f, 0.f);
        TileItem.UV1 = FVector2D(1.f, 1.f);
        TileItem.MaterialRenderProxy = Target.BrushMID->GetRenderProxy();
        TileItem.BlendMode = SE_BLEND_Additive; // multiple brushes accumulate in RT
        Canvas->DrawItem(TileItem);
    }

    UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, Context);
}

bool UMainOcclusionPainter::RefreshFrustumParams()
{
    if (!IsValid(PlayerController)) return false;

    return FFrustumProjectionMatcherHelper::ExtractFromCameraManager(
        PlayerController->PlayerCameraManager,
        FrustumParams);
}

bool UMainOcclusionPainter::ShouldRunClientLogic()
{
    if (GetNetMode() == NM_DedicatedServer)
        return false;

    return true;
}
