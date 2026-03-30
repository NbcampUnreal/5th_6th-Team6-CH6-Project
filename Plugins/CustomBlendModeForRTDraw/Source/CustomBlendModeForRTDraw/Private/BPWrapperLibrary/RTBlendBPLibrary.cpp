#include "BPWrapperLibrary/RTBlendBPLibrary.h"
#include "RTBlendDraw/RTBlendDrawFunctions.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Debug/DebugHelper.h"

void URTBlendBPLibrary::ApplyMaterialParams(const FRTBlendMaterialParams& MaterialParams)
{
    UMaterialInstanceDynamic* MID = MaterialParams.Material;
    if (!MID) { return; }

    for (const FRTBlendScalarParam& P : MaterialParams.ScalarParams)
    {
        if (!P.ParamName.IsNone())
            MID->SetScalarParameterValue(P.ParamName, P.Value);
    }

    for (const FRTBlendVectorParam& P : MaterialParams.VectorParams)
    {
        if (!P.ParamName.IsNone())
            MID->SetVectorParameterValue(P.ParamName, P.Value);
    }

    for (const FRTBlendTextureParam& P : MaterialParams.TextureParams)
    {
        if (!P.ParamName.IsNone() && P.Texture)
            MID->SetTextureParameterValue(P.ParamName, P.Texture);
    }
}

void URTBlendBPLibrary::DrawFullscreenBlendPass(
    UObject*                WorldContext,
    FSourceRTPair           SourceRT,
    FRTBlendMaterialParams  MaterialParams)
{
    UWorld* World = GEngine->GetWorldFromContextObject(
        WorldContext, EGetWorldErrorMode::LogAndReturnNull);

    if (!World || !SourceRT.RT || !MaterialParams.Material)
    {
        UE_LOG(CustomBlendForRTDraw, Warning,
            TEXT("URTBlendBPLibrary::DrawFullscreenBlendPass >> Invalid params — World=%s RT=%s Material=%s"),
            World                   ? TEXT("ok") : TEXT("null"),
            SourceRT.RT             ? TEXT("ok") : TEXT("null"),
            MaterialParams.Material ? TEXT("ok") : TEXT("null"));
        return;
    }

    // Bind source RT to its material param for read-back
    if (!SourceRT.ParamName.IsNone())
        MaterialParams.Material->SetTextureParameterValue(SourceRT.ParamName, SourceRT.RT);

    ApplyMaterialParams(MaterialParams);
    FRTBlendDraw::DrawFullscreenBlendPass(World, SourceRT.RT, MaterialParams.Material);
}

void URTBlendBPLibrary::DrawRectBlendPass(
    UObject*                WorldContext,
    FSourceRTPair           SourceRT,
    FVector2D               ScreenPos,
    FVector2D               ScreenSize,
    FVector2D               UV0,
    FVector2D               UV1,
    FRTBlendMaterialParams  MaterialParams)
{
    UWorld* World = GEngine->GetWorldFromContextObject(
        WorldContext, EGetWorldErrorMode::LogAndReturnNull);

    if (!World || !SourceRT.RT || !MaterialParams.Material)
    {
        UE_LOG(CustomBlendForRTDraw, Warning,
            TEXT("URTBlendBPLibrary::DrawRectBlendPass >> Invalid params — World=%s RT=%s Material=%s"),
            World                   ? TEXT("ok") : TEXT("null"),
            SourceRT.RT             ? TEXT("ok") : TEXT("null"),
            MaterialParams.Material ? TEXT("ok") : TEXT("null"));
        return;
    }

    // Bind source RT to its material param for read-back
    if (!SourceRT.ParamName.IsNone())
        MaterialParams.Material->SetTextureParameterValue(SourceRT.ParamName, SourceRT.RT);

    ApplyMaterialParams(MaterialParams);
    FRTBlendDraw::DrawRectBlendPass(World, SourceRT.RT, MaterialParams.Material, ScreenPos, ScreenSize, UV0, UV1);
}