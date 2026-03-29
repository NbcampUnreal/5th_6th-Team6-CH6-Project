#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInterface.h"
#include "LOSResourcePoolSettings.generated.h"


// ── Visibility mesh material slot ────────────────────────────────────────────

/**
 * Maps a MeshKey to an ordered material array.
 * Materials must be listed in the same flat order that UVisibilityMeshComp::FindMeshesByTag
 * iterates — skeletal meshes first, then static, in component registration order.
 * One entry per material slot across all tagged meshes on the actor.
 */
USTRUCT(BlueprintType)
struct TOPDOWNVISION_API FLOSVisibilityMeshMaterialSlot
{
    GENERATED_BODY()

    /** Must match MeshKey on the target UVisibilityMeshComp. */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Visibility")
    FName MeshKey = NAME_None;

    /** Flat material array in FindMeshesByTag iteration order.
     *  One entry per material slot across all tagged meshes. */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Visibility")
    TArray<TSoftObjectPtr<UMaterialInterface>> Materials;
};


// ── Settings class ────────────────────────────────────────────────────────────

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

    /** Number of RT+MID pairs to pre-allocate on level start. */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Pool")
    int32 PreWarmCount = 20;

    /** If true, pool allocates a new slot when all are taken.
     *  If false, providers beyond PreWarmCount are skipped this frame. */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Pool")
    bool bGrowOnDemand = true;

    // ── Render Target ─────────────────────────────────────────────────────

    /** Template RT — pool reads SizeX, SizeY, RenderTargetFormat, ClearColor from this.
     *  Never drawn into directly. */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="RenderTarget")
    TSoftObjectPtr<UTextureRenderTarget2D> TemplateObstacleRT;

    // ── Materials ─────────────────────────────────────────────────────────

    /** Base material for pooled LOS stamp MIDs.
     *  One MID per pool slot created at pre-warm time. */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Materials")
    TSoftObjectPtr<UMaterialInterface> LOSStampMaterial;

    /** One entry per character type — keyed by MeshKey on UVisibilityMeshComp.
     *  Pool creates MIDs from the material array at acquire time. */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Materials")
    TArray<FLOSVisibilityMeshMaterialSlot> VisibilityMeshMaterialSlots;

#pragma region Param Names

    // ── Stamp MID params ──────────────────────────────────────────────────

    /** Texture param — obstacle mask RT bound into the stamp MID each frame. */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID Params|Stamp")
    FName StampObstacleTextureParam = TEXT("ObstacleMaskRT");

    /** Scalar param — normalized vision radius [0..0.5] set on range change. */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID Params|Stamp")
    FName StampNormalizedRadiusParam = TEXT("NormalizedVisionRadius");

    /** Scalar param — visibility alpha [0..1] set each frame. */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID Params|Stamp")
    FName StampVisibilityAlphaParam = TEXT("VisibilityAlpha");

    // ── MPC params ────────────────────────────────────────────────────────

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID Params|MPC")
    FName MPCVisionCenterLocation = TEXT("VisionCenterLocation");

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID Params|MPC")
    FName MPCVisibleRange = TEXT("VisibleRange");

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID Params|MPC")
    FName MPCNearSightRange = TEXT("NearSightRange");

    // ── Occlusion params ──────────────────────────────────────────────────

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID Params|Occlusion")
    FName OcclusionRevealAlphaParam = TEXT("RevealAlpha");

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID Params|Occlusion")
    FName OcclusionTilePosParam = TEXT("TilePos");

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID Params|Occlusion")
    FName OcclusionTileSizeParam = TEXT("TileSize");

    // ── Visibility mesh params ────────────────────────────────────────────

    /** Scalar param name on visibility mesh materials that controls fade (0~1). */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID Params|Visibility")
    FName VisibilityAlphaParam = TEXT("VisibilityAlpha");

    // ── Tags ──────────────────────────────────────────────────────────────

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Tags")
    FName VisibilityMeshTag = TEXT("VisibilityMesh");

#pragma endregion
};
