// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "CurvedWorldSubsystem.h"
#include "OcclusionProbeComp.generated.h"


/*
 *  This will be used for detecting the obstacle hiding the character on the camera view
 *
 *  This is made in a separate component,
 *  --> because there are cases when multiple actors need to be shown in one camera other than the player character
 *
 *  Uses sphere sweeps along the curved world path instead of persistent sphere components,
 *  which avoids broadphase maintenance cost of dynamic sphere repositioning every tick.
 *
 *  Hit results are diffed against the previous frame to synthesize Begin/End occlusion signals,
 *  which are then dispatched via IOcclusionInterface on hit actors.
 *
 *  Tick is disabled — call UpdateOcclusionProbe() manually for custom tick control.
 *
 *  Segment count is dynamic — derived from path length / SegmentLength, clamped between Min/MaxSegments.
 */

class APlayerCameraManager;

// Log
PROJECTER_API DECLARE_LOG_CATEGORY_EXTERN(OcclusionProbeComp, Log, All);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTER_API UOcclusionProbeComp : public USceneComponent // root is used for target (endpoint)
{
    GENERATED_BODY()

public:
    UOcclusionProbeComp();

protected:
    virtual void BeginPlay() override;

public:
    /** Call this manually from your custom tick — runs sweeps and processes hit diff */
    UFUNCTION(BlueprintCallable, Category="Occlusion")
    void UpdateOcclusionProbe();

    UFUNCTION(BlueprintCallable, Category="Occlusion")
    void SetCameraManager();

    UFUNCTION(BlueprintCallable, Category="Occlusion")
    void SetCameraManager_Test(APlayerCameraManager* InCameraManager) {CameraManager=InCameraManager;};

private:

    /** Run sweeps along the curved path and collect all hit primitive components */
    void RunSweeps();

    /** Diff current vs previous hit sets and dispatch interface calls */
    void ProcessHitDiff();

    /** Calculate world-space sphere radius matching the actor's screen footprint */
    float CalculateSweepRadius() const;

private:

    /** World-space distance between each sweep sphere — segment count is derived from this */
    UPROPERTY(EditAnywhere, Category="Occlusion")
    float SegmentLength = 100.f;

    /** Minimum number of segments regardless of distance */
    UPROPERTY(EditAnywhere, Category="Occlusion")
    int32 MinSegments = 2;

    /** Maximum number of segments to cap performance cost */
    UPROPERTY(EditAnywhere, Category="Occlusion")
    int32 MaxSegments = 20;

    UPROPERTY(EditAnywhere, Category="Occlusion")
    float MinSweepRadius = 25.f;

    UPROPERTY(EditAnywhere, Category="Occlusion")
    float MaxSweepRadius = 200.f;

    /** Collision channel the obstacle meshes respond to */
    UPROPERTY(EditAnywhere, Category="Occlusion")
    TEnumAsByte<ECollisionChannel> OcclusionProbeChannel = ECC_GameTraceChannel3;

    /** Draw debug spheres along the sweep path */
    UPROPERTY(EditAnywhere, Category="Occlusion|Debug")
    bool bDrawDebug = false;

    /** Components hit this frame */
    TSet<TWeakObjectPtr<UPrimitiveComponent>> CurrentHits;

    /** Components hit last frame — used for begin/end diff */
    TSet<TWeakObjectPtr<UPrimitiveComponent>> PreviousHits;

    UPROPERTY()
    APlayerCameraManager* CameraManager;

    UPROPERTY()
    UCurvedWorldSubsystem* CurvedWorldSubsystem;
};
