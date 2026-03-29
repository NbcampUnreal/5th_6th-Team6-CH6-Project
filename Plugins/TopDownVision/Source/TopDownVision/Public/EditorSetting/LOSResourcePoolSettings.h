#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInterface.h"
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

    /** Number of ObstacleRT + StampMID pairs to pre-allocate. */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Pool")
    int32 PreWarmCount = 20;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Pool")
    bool bGrowOnDemand = true;

    // ── Render Target ─────────────────────────────────────────────────────

    /** Template RT — pool reads format, size, clear color from this. Never drawn into. */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="RenderTarget")
    TSoftObjectPtr<UTextureRenderTarget2D> TemplateObstacleRT;

    // ── Materials ─────────────────────────────────────────────────────────

    /** Base material for pooled LOS stamp MIDs. */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Materials")
    TSoftObjectPtr<UMaterialInterface> LOSStampMaterial;

#pragma region Param Names

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID Params|Stamp")
    FName StampObstacleTextureParam = TEXT("ObstacleMaskRT");

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID Params|Stamp")
    FName StampNormalizedRadiusParam = TEXT("NormalizedVisionRadius");

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID Params|Stamp")
    FName StampVisibilityAlphaParam = TEXT("VisibilityAlpha");

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID Params|MPC")
    FName MPCVisionCenterLocation = TEXT("VisionCenterLocation");

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID Params|MPC")
    FName MPCVisibleRange = TEXT("VisibleRange");

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID Params|MPC")
    FName MPCNearSightRange = TEXT("NearSightRange");

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID Params|Occlusion")
    FName OcclusionRevealAlphaParam = TEXT("RevealAlpha");

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID Params|Occlusion")
    FName OcclusionTilePosParam = TEXT("TilePos");

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID Params|Occlusion")
    FName OcclusionTileSizeParam = TEXT("TileSize");

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID Params|Visibility")
    FName VisibilityAlphaParam = TEXT("VisibilityAlpha");

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Tags")
    FName VisibilityMeshTag = TEXT("VisibilityMesh");

#pragma endregion
};
