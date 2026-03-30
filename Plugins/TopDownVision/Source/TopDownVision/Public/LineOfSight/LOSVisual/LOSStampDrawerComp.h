#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LOSStampDrawerComp.generated.h"

class UTextureRenderTarget2D;
class UMaterialInterface;
class UMaterialInstanceDynamic;


/**
 * Binds the obstacle RT into the LOS raymarching MID each frame.
 * MainVisionRTManager draws the MID directly onto CameraLocalRT — no intermediate RT.
 *
 * Two operating modes:
 *
 *   Owned mode  (bUseResourcePool = false on Vision_VisualComp):
 *     CreateResources() creates a MID from LOSMaterial and keeps it for the actor's lifetime.
 *
 *   Pool mode   (bUseResourcePool = true):
 *     CreateResources() is NOT called. The pool subsystem calls SetStampMID() on slot acquire
 *     to inject a pre-created MID, and SetStampMID(nullptr) on release to strip it.
 *     LOSMaterial and per-component param name properties are unused in this mode —
 *     all config comes from LOSResourcePoolSettings.
 *
 * Server/client gating is the caller's responsibility.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API ULOSStampDrawerComp : public UActorComponent
{
    GENERATED_BODY()

public:
    ULOSStampDrawerComp();

protected:
    virtual void BeginPlay() override;

public:

    /** Bind the obstacle RT into the active MID each frame.
     *  Safe to call in both owned and pool mode — no-ops if MID is null. */
    UFUNCTION(BlueprintCallable, Category="LOSStamp")
    void UpdateLOSStamp(UTextureRenderTarget2D* ObstacleRT);

    /** Toggled by the RT manager based on camera visibility. */
    void ToggleLOSStampUpdate(bool bIsOn);
    bool IsUpdating() const { return bShouldUpdateLOSStamp; }

    /** Called when vision range changes to keep the material param in sync. */
    UFUNCTION(BlueprintCallable, Category="LOSStamp")
    void OnVisionRangeChanged(float NewRange, float MaxRange);

    /** Owned mode only — creates LOSMaterialMID from LOSMaterial. */
    void CreateResources();

    void SetVisionAlpha(float InAlpha) { PassedVisionAlpha = InAlpha; }

    // --- Pool mode --- //

    /** Called by Vision_VisualComp::OnPoolSlotAcquired/Released.
     *  Pass nullptr to strip the MID on slot release. */
    void SetStampMID(UMaterialInstanceDynamic* InMID);

    // --- Getter --- //
    UMaterialInstanceDynamic* GetLOSMaterialMID() const { return LOSMaterialMID; }

protected:

    /** Owned mode — base material to create MID from.
     *  Unused in pool mode (material comes from LOSResourcePoolSettings). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LOSStamp|Owned")
    UMaterialInterface* LOSMaterial = nullptr;

    UPROPERTY(Transient)
    UMaterialInstanceDynamic* LOSMaterialMID = nullptr;

    // --- Debug --- //
    UPROPERTY(EditAnywhere, Category="LOSStamp|Debug")
    bool bDrawStampRange = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LOSStamp|Debug")
    bool bDrawDebugRT = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LOSStamp|Debug")
    TObjectPtr<UTextureRenderTarget2D> DebugRT = nullptr;

private:
    bool  bShouldUpdateLOSStamp = false;
    float CachedVisionRange     = 0.f;
    float PassedVisionAlpha     = 0.f;
};
