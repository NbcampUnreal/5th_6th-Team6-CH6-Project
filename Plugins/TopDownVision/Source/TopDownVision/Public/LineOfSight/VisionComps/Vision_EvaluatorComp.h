// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Vision_EvaluatorComp.generated.h"


//=== FD ===//
class USphereComponent;
class UVision_VisualComp;
class UTopDown2DShapeComp;
class ULOSObstacleDrawerComponent;


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UVision_EvaluatorComp : public UActorComponent
{
	GENERATED_BODY()

public:
    UVision_EvaluatorComp();

protected:
    virtual void BeginPlay() override;

public:


    UFUNCTION(BlueprintCallable, Category="Vision")
    void Initialize();
    
    // --- Called externally --- //

    /** Start/stop evaluation tick. Toggled by the RT manager or game logic. */
    void SetEvaluationEnabled(bool bEnabled);


    /** Sync detection sphere radius from Vision_VisualComp. 
 *  Called at BeginPlay and whenever VisionRange changes. */
    void SyncDetectionRadius();

private:
    // --- Overlap callbacks --- //
    UFUNCTION()
    void OnDetectionSphereBeginOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

    UFUNCTION()
    void OnDetectionSphereEndOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex);

    // --- Evaluation --- //

    /** Runs every evaluation tick for all currently overlapping targets. */
    void EvaluateTick();

    /** Evaluate a single target — wall type, volume type, or both. */
    void EvaluateTarget(AActor* Target, UVision_VisualComp* TargetVisual);

    /** Wall type: fan-shaped multi-line traces against R channel. */
    bool EvaluateWallObstacle(AActor* Target, UTopDown2DShapeComp* ShapeComp);

    /** Volume type: sample inner points against G channel of obstacle RT. */
    bool EvaluateVolumeObstacle(AActor* Target, UTopDown2DShapeComp* ShapeComp);

    // --- Helpers --- //

    /** Try to get Vision_VisualComp from a target actor. */
    UVision_VisualComp* GetVisualComp(AActor* Target) const;

    bool ShouldRunServerLogic() const;

    void StartEvaluationTimer();
    void StopEvaluationTimer();

    void ReportVisibility(AActor* Target, bool bVisible);//report to subsystem

protected:
    // --- Detection sphere --- //
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Evaluator")
    USphereComponent* DetectionSphere = nullptr;

    // --- Detection Channel --- //
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Evaluator")
    TEnumAsByte<ECollisionChannel> VisionTargetChannel = ECC_GameTraceChannel1;// use custom in BP
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Evaluator")
    float DetectionRadius = 1200.f;


    //Evaluation

    //Volume type occlusion threshold
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Evaluator")
    float OcclusionThreshold=0.9f;// 90%

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Evaluator")
    TEnumAsByte<ECollisionChannel> WallTraceChannel = ECC_Visibility;

    // --- Evaluation interval --- //
    /** How often evaluation runs while targets are overlapping. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Evaluator", meta=(ClampMin="0.01"))
    float EvaluationInterval = 0.1f;

    // --- Tag filter --- //
    /** Only actors with this tag on their overlap component will be evaluated. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Evaluator")
    FName TargetTag = TEXT("VisionTarget");

    // --- Debug --- //
    UPROPERTY(EditAnywhere, Category="Evaluator|Debug")
    bool bDrawDebug = false;

private:
    /** Currently overlapping targets being evaluated. */
    UPROPERTY(Transient)
    TSet<AActor*> OverlappingTargets;

    FTimerHandle EvaluationTimerHandle;
    
    /** Cached reference to sibling Vision_VisualComp. Resolved at BeginPlay. */
    UPROPERTY(Transient)
    UVision_VisualComp* CachedVisualComp = nullptr;
};