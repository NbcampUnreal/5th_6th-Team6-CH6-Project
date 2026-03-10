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

void UMainOcclusionPainter::InitializeOcclusionComponent()
{
    if (GetNetMode() == NM_DedicatedServer)
    {
        Deactivate();
        return;
    }

    Activate();

    if (BrushMaterial)
    {
        BrushMID = UMaterialInstanceDynamic::Create(BrushMaterial, this);
    }
    else
    {
        UE_LOG(OcclusionPainter, Warning,
            TEXT("UMainOcclusionPainter::BeginPlay>> No BrushMaterial set on %s"),
            *GetOwner()->GetName());
    }
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
    if (!bReady) return;
    if (!IsActive()) return;
    if (!IsValid(OcclusionRT)) return;
    if (!IsValid(BrushMID)) return;
    if (!RefreshFrustumParams()) return;

    UKismetRenderingLibrary::ClearRenderTarget2D(
        this, OcclusionRT, FLinearColor::Black);

    DrawProviderArea();
}

void UMainOcclusionPainter::DrawProviderArea()
{
    if (!IsValid(PlayerController)) return;

    UOcclusionSubsystem* Sub = GetWorld()->GetSubsystem<UOcclusionSubsystem>();
    if (!Sub) return;

    const TArray<FOcclusionBrushTarget>& Targets = Sub->GetTargets();
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

    for (const FOcclusionBrushTarget& Target : Targets)
    {
        if (!Target.IsValid()) continue;

        const FVector WorldPos    = Target.PrimitiveComp->GetComponentLocation();
        const float VisibleRadius = Target.GetRadius();

        FVector2D ScreenPos;
        if (!PlayerController->ProjectWorldLocationToScreen(WorldPos, ScreenPos))
            continue;

        const FVector2D ScreenUV = ScreenPos / FVector2D(VPX, VPY);

        const float TargetDistance = FVector::Dist(
            FrustumParams.CameraLocation, WorldPos);

        if (TargetDistance <= KINDA_SMALL_NUMBER) continue;

        const float NormalizedRadius = FFrustumProjectionMatcherHelper::ScreenRadiusToWorldRadius(
            FrustumParams,
            VisibleRadius / TargetDistance,
            TargetDistance);

        const float BrushSizeRT = NormalizedRadius * RTHeight;

        if (BrushSizeRT <= 0.f) continue;

        // Only param needed — position and size handled by tile item
        BrushMID->SetScalarParameterValue(RevealAlphaParam, Target.RevealAlpha);

        const FVector2D BrushPos(
            ScreenUV.X * RTWidth  - BrushSizeRT * 0.5f,
            ScreenUV.Y * RTHeight - BrushSizeRT * 0.5f);

        FCanvasTileItem TileItem(
            BrushPos,
            GWhiteTexture,
            FVector2D(BrushSizeRT, BrushSizeRT),
            FLinearColor::White);

        TileItem.MaterialRenderProxy = BrushMID->GetRenderProxy();
        TileItem.BlendMode = SE_BLEND_Additive;
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