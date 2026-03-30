#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInterface.h"
#include "LOSResourcePoolSettings.generated.h"


// ── Per-slot material entry ───────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct TOPDOWNVISION_API FLOSMIDMaterialEntry
{
    GENERATED_BODY()

    /** Material slot index on the skeletal mesh. */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID")
    int32 SlotIndex = 0;

    /** Material to create a MID from for this slot. */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID")
    TSoftObjectPtr<UMaterialInterface> Material;
};


// ── Visibility mesh MID pool entry ───────────────────────────────────────────

/**
 * One entry per monster type.
 * MeshKey must match the key set via UVisibilityMeshComp::SetMeshKey().
 * PoolCount is fixed — never grows. If exhausted, actor keeps original materials.
 * Materials are specified as (SlotIndex, Material) pairs — no mesh asset needed.
 */
USTRUCT(BlueprintType)
struct TOPDOWNVISION_API FLOSVisibilityMeshMaterialSlot
{
    GENERATED_BODY()

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID")
    FName MeshKey = NAME_None;

    /** Fixed pool size — max concurrent visible actors of this type. Never grows. */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID",
              meta=(ClampMin="1", ClampMax="128"))
    int32 PoolCount = 8;

    /** Materials paired with their mesh slot index. */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="MID")
    TArray<FLOSMIDMaterialEntry> Materials;
};


// ── Settings ─────────────────────────────────────────────────────────────────

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

    // ── RT + StampMID pool ────────────────────────────────────────────────

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Pool")
    int32 PreWarmCount = 20;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Pool")
    bool bGrowOnDemand = true;

    // ── Render Target ─────────────────────────────────────────────────────

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="RenderTarget")
    TSoftObjectPtr<UTextureRenderTarget2D> TemplateObstacleRT;

    // ── Stamp material ────────────────────────────────────────────────────

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Materials")
    TSoftObjectPtr<UMaterialInterface> LOSStampMaterial;

    // ── Visibility MID pools — one entry per monster type ─────────────────

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Materials")
    TArray<FLOSVisibilityMeshMaterialSlot> VisibilityMeshMaterialSlots;

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
