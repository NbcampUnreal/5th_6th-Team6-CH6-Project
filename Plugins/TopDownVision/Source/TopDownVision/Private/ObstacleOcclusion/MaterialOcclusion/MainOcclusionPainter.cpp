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

    const float ScreenAspect = static_cast<float>(VPX) / static_cast<float>(VPY);
    const float RTAspect     = RTWidth / RTHeight;
    const float AspectScale  = ScreenAspect / RTAspect;

    for (FOcclusionBrushTarget& Target : Targets)//batch the targets occlusion brushes
    {
        if (!Target.IsValid()) continue;

        if (!Target.IsMIDReady())
            Target.InitializeMID(this, DefaultBrushMaterial);

        if (!Target.IsMIDReady()) continue;

        const FVector WorldPos    = Target.PrimitiveComp->GetComponentLocation();
        const float VisibleRadius = Target.GetVisibleRadius();

        FVector2D ScreenPos;
        if (!PlayerController->ProjectWorldLocationToScreen(WorldPos, ScreenPos))
            continue;

        const float TargetDistance = FVector::Dist(FrustumParams.CameraLocation, WorldPos);
        if (TargetDistance <= KINDA_SMALL_NUMBER) continue;

        // Compute size first so we can use it for the bounds margin
        const float NormalizedRadius = VisibleRadius /
            (FMath::Tan(FrustumParams.FrustumHalfAngleRad) * TargetDistance);

        const float BrushSizeRT_Y = NormalizedRadius * RTHeight;
        const float BrushSizeRT_X = BrushSizeRT_Y / AspectScale;

        if (BrushSizeRT_Y <= 0.f) continue;

        // Cull targets whose brush rect is fully outside the RT
        const FVector2D ScreenUV = ScreenPos / FVector2D(VPX, VPY);
        const float MarginX = BrushSizeRT_X / RTWidth;
        const float MarginY = BrushSizeRT_Y / RTHeight;

        if (ScreenUV.X + MarginX < 0.f || ScreenUV.X - MarginX > 1.f ||
            ScreenUV.Y + MarginY < 0.f || ScreenUV.Y - MarginY > 1.f)
            continue;

        const FVector2D BrushPos(
            ScreenUV.X * RTWidth  - BrushSizeRT_X * 0.5f,
            ScreenUV.Y * RTHeight - BrushSizeRT_Y * 0.5f);

        Target.BrushMID->SetScalarParameterValue(RevealAlphaParam, Target.RevealAlpha);

        Target.BrushMID->SetVectorParameterValue(
            TEXT("TilePos"),
            FLinearColor(BrushPos.X / RTWidth, BrushPos.Y / RTHeight, 0.f, 0.f));
        Target.BrushMID->SetVectorParameterValue(
            TEXT("TileSize"),
            FLinearColor(BrushSizeRT_X / RTWidth, BrushSizeRT_Y / RTHeight, 0.f, 0.f));

        const FMaterialRenderProxy* RenderProxy = Target.BrushMID->GetRenderProxy();
        if (!RenderProxy) continue;

        FCanvasTileItem TileItem(
            BrushPos,
            RenderProxy,
            FVector2D(BrushSizeRT_X, BrushSizeRT_Y));

        TileItem.UV0 = FVector2D(0.f, 0.f);
        TileItem.UV1 = FVector2D(1.f, 1.f);
        TileItem.BlendMode = SE_BLEND_Additive;
        Canvas->DrawItem(TileItem);
    }

    // all drawn
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
