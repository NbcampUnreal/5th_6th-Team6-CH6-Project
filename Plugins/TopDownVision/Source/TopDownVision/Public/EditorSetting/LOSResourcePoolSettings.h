// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/TextureRenderTarget2D.h"
#include "LOSResourcePoolSettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="LOS Resource Pool Settings"))
class TOPDOWNVISION_API ULOSResourcePoolSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    ULOSResourcePoolSettings()
    {
        CategoryName = TEXT("TopDownVision");
        SectionName  = TEXT("LOS Resource Pool");
    }

    static const ULOSResourcePoolSettings* Get()
    {
        return GetDefault<ULOSResourcePoolSettings>();
    }

    // ── Pool ─────────────────────────────────────────────────────────────

    /** Number of RT+MID pairs to pre-allocate on level start */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Pool")
    int32 PreWarmCount = 20;

    // ── Render Target ─────────────────────────────────────────────────────

    /** Resolution for all LOS render targets */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="RenderTarget")
    int32 RTResolution = 256;

    /** Format for all LOS render targets */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="RenderTarget")
    TEnumAsByte<ETextureRenderTargetFormat> RTFormat = RTF_RGBA8;

    // ── Stamp MID params ─────────────────────────────────────────────────

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID Params|Stamp")
    FName StampRenderTargetParam = TEXT("RenderTarget");

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID Params|Stamp")
    FName StampInputTextureParam = TEXT("InputTexture");

    // ── MPC params ───────────────────────────────────────────────────────

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID Params|MPC")
    FName MPCVisionCenterLocation = TEXT("VisionCenterLocation");

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID Params|MPC")
    FName MPCVisibleRange = TEXT("VisibleRange");

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID Params|MPC")
    FName MPCNearSightRange = TEXT("NearSightRange");

    // ── Occlusion params ─────────────────────────────────────────────────

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID Params|Occlusion")
    FName OcclusionRevealAlphaParam = TEXT("RevealAlpha");

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID Params|Occlusion")
    FName OcclusionTilePosParam = TEXT("TilePos");

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID Params|Occlusion")
    FName OcclusionTileSizeParam = TEXT("TileSize");

    // ── Visibility mesh params ────────────────────────────────────────────

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID Params|Visibility")
    FName VisibilityAlphaParam = TEXT("VisibilityAlpha");

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Tags")
    FName VisibilityMeshTag = TEXT("VisibilityMesh");
};