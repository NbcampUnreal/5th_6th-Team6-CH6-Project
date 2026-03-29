#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LOSObstacleDrawerComponent.generated.h"

class UTextureRenderTarget2D;
class ULocalTextureSampler;


/**
 * Samples the pre-baked world obstacle texture into a local RenderTarget
 * each LOS frame, and exposes the resulting RT for the LOS stamp painter.
 *
 * Two initialization paths:
 *
 *   Owned mode  — Initialize(MaxVisionRange):
 *     Creates its own ObstacleRT and keeps it for the actor's lifetime.
 *
 *   Pool mode   — InitializeSamplerOnly(MaxVisionRange):
 *     Sets up LocalTextureSampler (world, root, radius) without allocating an RT.
 *     The pool subsystem later calls SetLocalRenderTargetOnly on the sampler
 *     to hand in a pooled RT on slot acquire and null it on release.
 *
 * Server/client gating is the caller's responsibility.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API ULOSObstacleDrawerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULOSObstacleDrawerComponent();

protected:
    virtual void BeginPlay() override;

public:

    /** Owned mode — sets range, creates ObstacleRT, fully initializes sampler. */
    UFUNCTION(BlueprintCallable, Category="LOSObstacle")
    void Initialize(float InMaxVisionRange);

    /** Pool mode — sets range and wires sampler root/world, but does NOT create an RT.
     *  The pool subsystem assigns the RT later via SetLocalRenderTargetOnly. */
    UFUNCTION(BlueprintCallable, Category="LOSObstacle")
    void InitializeSamplerOnly(float InMaxVisionRange);

    /** Sample the pre-baked obstacle texture into the active RT.
     *  Works for both owned and pool mode — reads RT from the sampler directly.
     *  Caller is responsible for net role gating. */
    UFUNCTION(BlueprintCallable, Category="LOSObstacle")
    void UpdateObstacleTexture();

    /** Returns the active RT — sampler's LocalMaskRT in pool mode,
     *  ObstacleRenderTarget in owned mode. */
    UFUNCTION(BlueprintCallable, Category="LOSObstacle")
    UTextureRenderTarget2D* GetObstacleRenderTarget() const;

    UFUNCTION(BlueprintCallable, Category="LOSObstacle")
    ULocalTextureSampler* GetLocalTextureSampler() const { return LocalTextureSampler; }

    /** Owned mode only — called by Initialize internally. */
    UFUNCTION(BlueprintCallable, Category="LOSObstacle")
    void CreateResources();

private:

    /** Shared setup — wires sampler world, root, and radius. Used by both init paths. */
    void SetupSampler();

protected:

    UPROPERTY(EditAnywhere, Category="LOSObstacle|Debug")
    bool bDrawSampleRange = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LOSObstacle")
    ULocalTextureSampler* LocalTextureSampler = nullptr;

    /** Valid in owned mode only. Null in pool mode — RT lives on the sampler. */
    UPROPERTY(Transient)
    UTextureRenderTarget2D* ObstacleRenderTarget = nullptr;

    /** Passed in from Vision_VisualComp before resources are created. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LOSObstacle")
    float MaxVisionRange = 0.f;
};
