#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LOSObstacleDrawerComponent.generated.h"

class UTextureRenderTarget2D;
class ULocalTextureSampler;
class UMaterialInterface;
class UMaterialInstanceDynamic;


/**
 * Samples the pre-baked world obstacle texture into a local RenderTarget
 * each LOS frame, and exposes the resulting RT and MID for the LOS painter.
 *
 * This component has no concept of vision range or line-of-sight logic.
 * It only knows how wide the world-space sampling window is (SampleWorldWidth)
 * and how large the RT should be (PixelResolution).
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
    /** Called by VisionTargetComp::BeginPlay. Sets the RT world coverage and creates resources.
     *  MaxVisionRange must come from VisionTargetComp — it is not editable here. */
    void Initialize(float InMaxVisionRange);

    /** Sample the pre-baked obstacle texture into the local RT.
     *  Caller is responsible for only invoking this on the correct net role. */
    UFUNCTION(BlueprintCallable, Category="LOSObstacle")
    void UpdateObstacleTexture();

    /** The RT that holds the merged obstacle frame for this actor. */
    UFUNCTION(BlueprintCallable, Category="LOSObstacle")
    UTextureRenderTarget2D* GetObstacleRenderTarget() const { return ObstacleRenderTarget; }

    /*/** The MID whose texture parameter is bound to the obstacle RT. #1#
    UFUNCTION(BlueprintCallable, Category="LOSObstacle")
    UMaterialInstanceDynamic* GetObstacleMID() const { return ObstacleMaterialMID; }*/
    
    //  Private helpers
private:
    void CreateResources();
    
    //  Properties
protected:
    // --- Debug ---
    UPROPERTY(EditAnywhere, Category="LOSObstacle|Debug")
    bool bDrawSampleRange = false;

    // --- Sampling ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LOSObstacle")
    ULocalTextureSampler* LocalTextureSampler = nullptr;

    UPROPERTY(Transient)
    UTextureRenderTarget2D* ObstacleRenderTarget = nullptr;

    /** How many pixels wide/tall the local RT is. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LOSObstacle")
    int32 PixelResolution = 256;

    /** Passed in from VisionTargetComp before BeginPlay resources are created. */
    float MaxVisionRange = 0.f;

    // material merge is not used any more
    /*// --- Material ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LOSObstacle")
    UMaterialInterface* ObstacleMaterial = nullptr;

    UPROPERTY(Transient)
    UMaterialInstanceDynamic* ObstacleMaterialMID = nullptr;

    /** Name of the texture parameter in ObstacleMaterial to bind the RT to. #1#
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LOSObstacle")
    FName MIDTextureParam = TEXT("ObstacleMaskRT");*/
};
