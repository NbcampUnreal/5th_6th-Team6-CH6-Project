#pragma once

#include "CoreMinimal.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/DeveloperSettings.h"
#include "RTPoolTypes.generated.h"

USTRUCT(BlueprintType)
struct MATERIALDRIVENINTERACTION_API FRTPoolEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Foliage RT")
    TObjectPtr<UTextureRenderTarget2D> InteractionRT = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Foliage RT")
    FIntPoint AssignedCell = FIntPoint(INT32_MIN, INT32_MIN);

    UPROPERTY(BlueprintReadOnly, Category = "Foliage RT")
    FVector2D CellOriginWS = FVector2D(-9999999.f, -9999999.f);

    UPROPERTY(BlueprintReadOnly, Category = "Foliage RT")
    float LastReleaseTime = -1.f;

    UPROPERTY(BlueprintReadOnly, Category = "Foliage RT")
    int32 ActiveInvokerCount = 0;

    float LastDecayRate = 0.95f;
    bool  bNeedsClear   = false;

    bool IsFree()     const { return AssignedCell == FIntPoint(INT32_MIN, INT32_MIN); }
    bool IsOccupied() const { return !IsFree(); }
};

UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Foliage RT Pool"))
class MATERIALDRIVENINTERACTION_API URTPoolSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    URTPoolSettings()
    {
        CategoryName  = TEXT("MaterialDrivenInteraction");
        SectionName   = TEXT("Foliage RT Pool");
        PoolSize      = 9;
        CellSize      = 2000.f;
        RTResolution  = 512;
        DecayDuration = 10.f;
        MaxVelocity   = 1200.f;
        Threshold     = 0.0393f;
    }

    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Pool",
        meta = (ClampMin = 1, ClampMax = 16))
    int32 PoolSize;

    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Grid",
        meta = (ClampMin = 100.f))
    float CellSize;

    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Pool",
        meta = (ClampMin = 64, ClampMax = 2048))
    int32 RTResolution;

    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Pool",
        meta = (ClampMin = 0.f))
    float DecayDuration;

    /** Max world units per second for velocity encoding. Must match across all invokers. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Invoker",
        meta = (ClampMin = 1.f))
    float MaxVelocity;

    /** Progression snap threshold — values below this snap to zero.
     *  Based on 8-bit pack precision: 10.0 / 255.0 = 0.0393 */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Invoker",
        meta = (ClampMin = 0.f))
    float Threshold;

    /** Per-frame progression decay rate. 0.95 = 5% decay per frame.
 *  Shared across all invokers — RT decay is per slot not per brush. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Invoker",
        meta = (ClampMin = 0.f, ClampMax = 1.f))
    float DecayRate;

    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Pool|RenderTargets")
    TArray<TSoftObjectPtr<UTextureRenderTarget2D>> InteractionRTs;

    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Pool|Materials")
    TSoftObjectPtr<UMaterialInterface> DecayMaterial;
};