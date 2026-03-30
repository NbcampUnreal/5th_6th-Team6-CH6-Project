#include "RTBlendDraw/RTBlendDrawFunctions.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "Debug/DebugHelper.h"

// ── Session API ───────────────────────────────────────────────────────────────

bool FRTBlendDraw::BeginBlendPass(
    UWorld*                     World,
    UTextureRenderTarget2D*     RT,
    UMaterialInstanceDynamic*   BlendMaterial,
    UCanvas*&                   OutCanvas,
    FVector2D&                  OutCanvasSize,
    FDrawToRenderTargetContext& OutContext)
{
    OutCanvas     = nullptr;
    OutCanvasSize = FVector2D::ZeroVector;

    if (!World || !RT || !BlendMaterial)
    {
        UE_LOG(CustomBlendForRTDraw, Warning,
            TEXT("FRTBlendDraw::BeginBlendPass >> Invalid params — World=%s RT=%s Material=%s"),
            World         ? TEXT("ok") : TEXT("null"),
            RT            ? TEXT("ok") : TEXT("null"),
            BlendMaterial ? TEXT("ok") : TEXT("null"));
        return false;
    }

    UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(
        World, RT, OutCanvas, OutCanvasSize, OutContext);

    if (!OutCanvas)
    {
        UE_LOG(CustomBlendForRTDraw, Warning,
            TEXT("FRTBlendDraw::BeginBlendPass >> Failed to get Canvas from RT '%s'"),
            *RT->GetName());
        UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(World, OutContext);
        return false;
    }

    UE_LOG(CustomBlendForRTDraw, Verbose,
        TEXT("FRTBlendDraw::BeginBlendPass >> Opened — RT='%s' CanvasSize=(%.0f,%.0f)"),
        *RT->GetName(), OutCanvasSize.X, OutCanvasSize.Y);

    return true;
}

void FRTBlendDraw::DrawRect(
    UCanvas*                  Canvas,
    UMaterialInstanceDynamic* BlendMaterial,
    FVector2D                 ScreenPos,
    FVector2D                 ScreenSize,
    FVector2D                 UV0,
    FVector2D                 UV1)
{
    if (!Canvas || !BlendMaterial)
    {
        UE_LOG(CustomBlendForRTDraw, Warning,
            TEXT("FRTBlendDraw::DrawRect >> Invalid params — Canvas=%s Material=%s"),
            Canvas        ? TEXT("ok") : TEXT("null"),
            BlendMaterial ? TEXT("ok") : TEXT("null"));
        return;
    }

    FCanvasTileItem TileItem(
        ScreenPos,
        BlendMaterial->GetRenderProxy(),
        ScreenSize);

    TileItem.UV0       = UV0;
    TileItem.UV1       = UV1;
    TileItem.BlendMode = SE_BLEND_Opaque;

    Canvas->DrawItem(TileItem);

    UE_LOG(CustomBlendForRTDraw, Verbose,
        TEXT("FRTBlendDraw::DrawRect >> Pos=(%.1f,%.1f) Size=(%.1f,%.1f)"),
        ScreenPos.X, ScreenPos.Y, ScreenSize.X, ScreenSize.Y);
}

void FRTBlendDraw::DrawFullscreen(
    UCanvas*                  Canvas,
    FVector2D                 CanvasSize,
    UMaterialInstanceDynamic* BlendMaterial)
{
    if (!Canvas || !BlendMaterial)
    {
        UE_LOG(CustomBlendForRTDraw, Warning,
            TEXT("FRTBlendDraw::DrawFullscreen >> Invalid params — Canvas=%s Material=%s"),
            Canvas        ? TEXT("ok") : TEXT("null"),
            BlendMaterial ? TEXT("ok") : TEXT("null"));
        return;
    }

    FCanvasTileItem TileItem(
        FVector2D(0.f, 0.f),
        BlendMaterial->GetRenderProxy(),
        CanvasSize);

    TileItem.UV0       = FVector2D(0.f, 0.f);
    TileItem.UV1       = FVector2D(1.f, 1.f);
    TileItem.BlendMode = SE_BLEND_Opaque;

    Canvas->DrawItem(TileItem);

    UE_LOG(CustomBlendForRTDraw, Verbose,
        TEXT("FRTBlendDraw::DrawFullscreen >> CanvasSize=(%.0f,%.0f)"),
        CanvasSize.X, CanvasSize.Y);
}

void FRTBlendDraw::EndBlendPass(
    UWorld*                    World,
    FDrawToRenderTargetContext& Context)
{
    if (!World)
    {
        UE_LOG(CustomBlendForRTDraw, Warning,
            TEXT("FRTBlendDraw::EndBlendPass >> World is null"));
        return;
    }

    UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(World, Context);

    UE_LOG(CustomBlendForRTDraw, Verbose,
        TEXT("FRTBlendDraw::EndBlendPass >> Closed"));
}

// ── Convenience API ───────────────────────────────────────────────────────────

void FRTBlendDraw::DrawFullscreenBlendPass(
    UWorld*                   World,
    UTextureRenderTarget2D*   RT,
    UMaterialInstanceDynamic* BlendMaterial)
{
    if (!World || !RT || !BlendMaterial)
    {
        UE_LOG(CustomBlendForRTDraw, Warning,
            TEXT("FRTBlendDraw::DrawFullscreenBlendPass >> Invalid params — World=%s RT=%s Material=%s"),
            World         ? TEXT("ok") : TEXT("null"),
            RT            ? TEXT("ok") : TEXT("null"),
            BlendMaterial ? TEXT("ok") : TEXT("null"));
        return;
    }

    UCanvas* Canvas = nullptr;
    FVector2D CanvasSize;
    FDrawToRenderTargetContext Context;

    UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(
        World, RT, Canvas, CanvasSize, Context);

    if (Canvas)
    {
        DrawFullscreen(Canvas, CanvasSize, BlendMaterial);
        UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(World, Context);

        UE_LOG(CustomBlendForRTDraw, Verbose,
            TEXT("FRTBlendDraw::DrawFullscreenBlendPass >> RT='%s'"),
            *RT->GetName());
    }
}

void FRTBlendDraw::DrawRectBlendPass(
    UWorld*                   World,
    UTextureRenderTarget2D*   RT,
    UMaterialInstanceDynamic* BlendMaterial,
    FVector2D                 ScreenPos,
    FVector2D                 ScreenSize,
    FVector2D                 UV0,
    FVector2D                 UV1)
{
    if (!World || !RT || !BlendMaterial)
    {
        UE_LOG(CustomBlendForRTDraw, Warning,
            TEXT("FRTBlendDraw::DrawRectBlendPass >> Invalid params — World=%s RT=%s Material=%s"),
            World         ? TEXT("ok") : TEXT("null"),
            RT            ? TEXT("ok") : TEXT("null"),
            BlendMaterial ? TEXT("ok") : TEXT("null"));
        return;
    }

    UCanvas* Canvas = nullptr;
    FVector2D CanvasSize;
    FDrawToRenderTargetContext Context;

    UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(
        World, RT, Canvas, CanvasSize, Context);

    if (Canvas)
    {
        DrawRect(Canvas, BlendMaterial, ScreenPos, ScreenSize, UV0, UV1);
        UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(World, Context);

        UE_LOG(CustomBlendForRTDraw, Verbose,
            TEXT("FRTBlendDraw::DrawRectBlendPass >> RT='%s' Pos=(%.1f,%.1f) Size=(%.1f,%.1f)"),
            *RT->GetName(), ScreenPos.X, ScreenPos.Y, ScreenSize.X, ScreenSize.Y);
    }
}