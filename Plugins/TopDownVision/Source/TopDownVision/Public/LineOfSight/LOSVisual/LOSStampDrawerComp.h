// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LineOfSight/VisionData.h"
#include "LOSStampDrawerComp.generated.h"

class UTextureRenderTarget2D;
class UMaterialInterface;
class UMaterialInstanceDynamic;


/**
 * Responsible solely for rendering the local LOS raymarching stamp.
 * Takes the obstacle RT from ULOSObstacleDrawerComponent as input,
 * binds it into the LOS material, and produces the final LOS stamp RT.
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
    /** Render the LOS stamp for this frame using the provided obstacle RT.
     *  Caller is responsible for only invoking this on the correct net role. */
    UFUNCTION(BlueprintCallable, Category="LOSStamp")
    void UpdateLOSStamp(UTextureRenderTarget2D* ObstacleRT);

    /** Toggled by the RT manager based on camera visibility. */
    void ToggleLOSStampUpdate(bool bIsOn);
    bool IsUpdating() const { return bShouldUpdateLOSStamp; }

    /** Called when vision range changes to keep the material param in sync. */
    UFUNCTION(BlueprintCallable, Category="LOSStamp")
    void OnVisionRangeChanged(float NewRange, float MaxRange);

    // --- Getters --- //
    UTextureRenderTarget2D*   GetLOSRenderTarget() const { return LOSRenderTarget; }
    UMaterialInstanceDynamic* GetLOSMaterialMID()  const { return LOSMaterialMID; }

    
    void CreateResources();// now make it public function for manual creation
private:
    

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LOSStamp")
    int32 PixelResolution = 256;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LOSStamp")
    UTextureRenderTarget2D* LOSRenderTarget = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LOSStamp")
    UMaterialInterface* LOSMaterial = nullptr;

    UPROPERTY(Transient)
    UMaterialInstanceDynamic* LOSMaterialMID = nullptr;

    /** Texture parameter name for the obstacle RT input in the LOS material. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LOSStamp")
    FName MIDObstacleTextureParam = TEXT("ObstacleMaskRT");

    /** Scalar parameter name for the normalized vision range in the LOS material. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LOSStamp")
    FName MIDVisibleRangeParam = TEXT("NormalizedVisionRadius");

    // --- Debug --- //
    UPROPERTY(EditAnywhere, Category="LOSStamp|Debug")
    bool bDrawStampRange = false;

private:
    bool  bShouldUpdateLOSStamp = false;
    float CachedVisionRange = 0.f; // for debug draw only — source of truth is VisionTargetComp

};
