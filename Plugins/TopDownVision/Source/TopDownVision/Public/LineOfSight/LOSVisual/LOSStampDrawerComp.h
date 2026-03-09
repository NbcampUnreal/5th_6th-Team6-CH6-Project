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
 * Responsible solely for binding the obstacle RT into the LOS raymarching material.
 * MainVisionRTManager draws the MID directly onto the camera RT each frame —
 * no intermediate stamp RT needed.
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
    /** Bind the obstacle RT into the MID each frame.
     *  No pre-render — MainVisionRTManager draws the MID directly. */
    UFUNCTION(BlueprintCallable, Category="LOSStamp")
    void UpdateLOSStamp(UTextureRenderTarget2D* ObstacleRT);

    /** Toggled by the RT manager based on camera visibility. */
    void ToggleLOSStampUpdate(bool bIsOn);
    bool IsUpdating() const { return bShouldUpdateLOSStamp; }

    /** Called when vision range changes to keep the material param in sync. */
    UFUNCTION(BlueprintCallable, Category="LOSStamp")
    void OnVisionRangeChanged(float NewRange, float MaxRange);

    /** Called by Vision_VisualComp::BeginPlay to create the MID. */
    void CreateResources();

    void SetVisionAlpha(float InAlpha) {PassedVisionAlpha=InAlpha;}

    // --- Getters --- //
    UMaterialInstanceDynamic* GetLOSMaterialMID() const { return LOSMaterialMID; }

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LOSStamp")
    UMaterialInterface* LOSMaterial = nullptr;

    UPROPERTY(Transient)
    UMaterialInstanceDynamic* LOSMaterialMID = nullptr;

    /** Texture parameter name for the obstacle RT in the LOS material. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LOSStamp")
    FName MIDObstacleTextureParam = TEXT("ObstacleMaskRT");

    /** Scalar parameter name for normalized vision range in the LOS material. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LOSStamp")
    FName MIDVisibleRangeParam = TEXT("NormalizedVisionRadius");

    /** Scalar parameter name for Opacity */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LOSStamp")
    FName MIDVisibilityAlpha = TEXT("VisibilityAlpha");

    // --- Debug --- //
    UPROPERTY(EditAnywhere, Category="LOSStamp|Debug")
    bool bDrawStampRange = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LOSStamp|Debug")
    bool bDrawDebugRT = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LOSStamp|Debug")
    TObjectPtr<UTextureRenderTarget2D> DebugRT = nullptr;

private:
    bool  bShouldUpdateLOSStamp = false;
    float CachedVisionRange = 0.f;

    float PassedVisionAlpha = 0.f;
};
