#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RTBlendDraw/RTBlendDrawFunctions.h"
#include "Engine/TextureRenderTarget2D.h"
#include "RTBlendBPLibrary.generated.h"

class UMaterialInstanceDynamic;
class UTexture;

#pragma region Params

// ── Per-type param structs ────────────────────────────────────────────────────

/** Source RT pair — write target and its material param name for read-back.
 *  RT is where the result is drawn to AND what the material reads from.
 *  ParamName is the Texture2D parameter name in the material. */
USTRUCT(BlueprintType)
struct FSourceRTPair
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ParamName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UTextureRenderTarget2D> RT = nullptr;
};

/** Named scalar param binding. */
USTRUCT(BlueprintType)
struct FRTBlendScalarParam
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ParamName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Value = 0.f;
};

/** Named vector param binding. */
USTRUCT(BlueprintType)
struct FRTBlendVectorParam
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ParamName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor Value = FLinearColor::Black;
};

/** Named texture param binding — for non-RT textures. */
USTRUCT(BlueprintType)
struct FRTBlendTextureParam
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ParamName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UTexture> Texture = nullptr;
};

// ── Material + additional params bundled ──────────────────────────────────────

/** Additional material params — scalars, vectors, and non-RT textures.
 *  Source RT is passed separately as FSourceRTPair. */
USTRUCT(BlueprintType)
struct FRTBlendMaterialParams
{
    GENERATED_BODY()

    /** The blend logic material instance. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UMaterialInstanceDynamic> Material = nullptr;

    /** Scalar parameter bindings. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FRTBlendScalarParam> ScalarParams;

    /** Vector parameter bindings. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FRTBlendVectorParam> VectorParams;

    /** Additional texture bindings — for non-RT textures e.g. shape masks. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FRTBlendTextureParam> TextureParams;
};

#pragma endregion

// ── BP Library ────────────────────────────────────────────────────────────────

UCLASS()
class CUSTOMBLENDMODEFORRTDRAW_API URTBlendBPLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /** Draw a fullscreen material pass onto a render target.
     *  SourceRT — write target and its param name for material read-back.
     *  MaterialParams — additional params applied to the material before drawing. */
    UFUNCTION(BlueprintCallable, Category = "RT Blend",
        meta = (WorldContext = "WorldContext"))
    static void DrawFullscreenBlendPass(
        UObject*                WorldContext,
        FSourceRTPair           SourceRT,
        FRTBlendMaterialParams  MaterialParams);

    /** Draw a rect material pass onto a render target.
     *  SourceRT — write target and its param name for material read-back.
     *  MaterialParams — additional params applied to the material before drawing. */
    UFUNCTION(BlueprintCallable, Category = "RT Blend",
        meta = (WorldContext = "WorldContext"))
    static void DrawRectBlendPass(
        UObject*                WorldContext,
        FSourceRTPair           SourceRT,
        FVector2D               ScreenPos,
        FVector2D               ScreenSize,
        FVector2D               UV0,
        FVector2D               UV1,
        FRTBlendMaterialParams  MaterialParams);

    // ── Channel mask helpers ──────────────────────────────────────────────────

    UFUNCTION(BlueprintPure, Category = "RT Blend|Channel")
    static uint8 MakeChannelMask(bool bR, bool bG, bool bB, bool bA)
    {
        return FRTBlendDraw::MakeChannelMask(bR, bG, bB, bA);
    }

    UFUNCTION(BlueprintPure, Category = "RT Blend|Channel")
    static float ChannelMaskToFloat(uint8 Mask)
    {
        return static_cast<float>(Mask);
    }

private:

    static void ApplyMaterialParams(const FRTBlendMaterialParams& MaterialParams);
};