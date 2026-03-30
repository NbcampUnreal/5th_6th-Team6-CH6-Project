#pragma once

#include "CoreMinimal.h"

// Forward declarations
struct FDrawToRenderTargetContext;
class UWorld;
class UTextureRenderTarget2D;
class UMaterialInstanceDynamic;
class UCanvas;

/**
 * FRTBlendDraw
 * Pure C++ static function library for drawing materials onto render targets
 * with custom blend logic via read-modify-write material passes.
 *
 * The material implements all blend logic — C++ only handles draw setup.
 * Caller is responsible for setting material params before calling draw functions.
 *
 * Usage (session — batched draws):
 *   UCanvas* Canvas; FVector2D CanvasSize; FDrawToRenderTargetContext Context;
 *   if (FRTBlendDraw::BeginBlendPass(World, RT, Material, Canvas, CanvasSize, Context))
 *   {
 *       FRTBlendDraw::DrawRect(Canvas, Material, Pos, Size, UV0, UV1);
 *       FRTBlendDraw::DrawFullscreen(Canvas, CanvasSize, Material);
 *       FRTBlendDraw::EndBlendPass(World, Context);
 *   }
 *
 * Usage (convenience — single draw):
 *   FRTBlendDraw::DrawFullscreenBlendPass(World, RT, Material);
 *   FRTBlendDraw::DrawRectBlendPass(World, RT, Material, Pos, Size, UV0, UV1);
 */
class CUSTOMBLENDMODEFORRTDRAW_API FRTBlendDraw
{
public:

    // ── Session API — batch multiple draws in one Begin/End ───────────────

    /** Open a blend pass session.
     *  Returns false if failed — do not call DrawRect or EndBlendPass. */
    static bool BeginBlendPass(
        UWorld*                     World,
        UTextureRenderTarget2D*     RT,
        UMaterialInstanceDynamic*   BlendMaterial,
        //out
        UCanvas*&                   OutCanvas,
        FVector2D&                  OutCanvasSize,
        FDrawToRenderTargetContext& OutContext);

    /** Draw a rect within an open blend pass session. */
    static void DrawRect(
        UCanvas*                  Canvas,
        UMaterialInstanceDynamic* BlendMaterial,
        FVector2D                 ScreenPos,
        FVector2D                 ScreenSize,
        FVector2D                 UV0,
        FVector2D                 UV1);

    /** Draw fullscreen within an open blend pass session. */
    static void DrawFullscreen(
        UCanvas*                  Canvas,
        FVector2D                 CanvasSize,
        UMaterialInstanceDynamic* BlendMaterial);

    /** Close the blend pass session. */
    static void EndBlendPass(
        UWorld*                    World,
        FDrawToRenderTargetContext& Context);

    // ── Convenience API — single draw, handles Begin/End internally ───────

    /** Single fullscreen draw. Not batchable.
     *  Caller must set material params before calling. */
    static void DrawFullscreenBlendPass(
        UWorld*                   World,
        UTextureRenderTarget2D*   RT,
        UMaterialInstanceDynamic* BlendMaterial);

    /** Single rect draw. Not batchable.
     *  Caller must set material params before calling. */
    static void DrawRectBlendPass(
        UWorld*                   World,
        UTextureRenderTarget2D*   RT,
        UMaterialInstanceDynamic* BlendMaterial,
        FVector2D                 ScreenPos,
        FVector2D                 ScreenSize,
        FVector2D                 UV0,
        FVector2D                 UV1);

#pragma region ChannelMaskHelper

    /** Build a channel mask from individual channel flags.
     *  Pass to material as float via SetScalarParameterValue.
     *  Example: MakeChannelMask(true, false, false, true) = 9 (R + A) */
    static uint8 MakeChannelMask(bool bR, bool bG, bool bB, bool bA)
    {
        return (bR ? 1 : 0)
             | (bG ? 2 : 0)
             | (bB ? 4 : 0)
             | (bA ? 8 : 0);
    }

#pragma endregion

private:

    // No BindRTs — param management is caller's responsibility
};