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
 *  which are then dispatched to IOcclusionTarget on hit actors.
 */

class APlayerCameraManager;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTER_API UOcclusionProbeComp : public USceneComponent
{
    GENERATED_BODY()

public:
    UOcclusionProbeComp();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

private:

    /** Run sweeps along the curved path and collect all hit primitive components */
    void RunSweeps();

    /** Diff current vs previous hit sets to fire Begin/End signals */
    void ProcessHitDiff();

    /** Calculate world-space sphere radius matching the actor's screen footprint */
    float CalculateSweepRadius() const;

private:

    /** Number of sweep segments along the curved path (camera -> character) */
    UPROPERTY(EditAnywhere, Category="Occlusion")
    int32 NumSegments = 8;

    UPROPERTY(EditAnywhere, Category="Occlusion")
    float MinSweepRadius = 25.f;

    UPROPERTY(EditAnywhere, Category="Occlusion")
    float MaxSweepRadius = 200.f;

<<<<<<< HEAD
    /** Collision channel the obstacle meshes respond to */
    UPROPERTY(EditAnywhere, Category="Occlusion")
    TEnumAsByte<ECollisionChannel> OcclusionProbeChannel = ECC_GameTraceChannel3;
=======
	UPROPERTY(EditAnywhere, Category="Occlusion")
	TEnumAsByte<ECollisionChannel> OcclusionProbeChannel = ECC_GameTraceChannel3;

	UPROPERTY()
	TArray<USphereComponent*> ProbeSpheres;
>>>>>>> feature_NewLOS

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
