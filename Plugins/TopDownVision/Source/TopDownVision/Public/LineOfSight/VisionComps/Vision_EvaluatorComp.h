#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Vision_EvaluatorComp.generated.h"

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
    void PrepareDetectionSphere();

    UFUNCTION(BlueprintCallable)
    void InitializeEvaluator(UVision_VisualComp* DirectParamComp);

    UFUNCTION(BlueprintCallable)
    void DirectCacheVisualComp(UVision_VisualComp* DirectParamComp);

    void FindAndCacheVisualComp();

    UFUNCTION(BlueprintCallable)
    void SetEvaluationEnabled(bool bEnabled);

    void SyncDetectionRadius();

    UFUNCTION(BlueprintCallable)
    void BP_DrawDebugSphereComp(float DrawTime);

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
    void EvaluateTick();
    void EvaluateTarget(AActor* Target, UVision_VisualComp* TargetVisual);
    bool EvaluateWallObstacle(AActor* Target, UTopDown2DShapeComp* ShapeComp);
    bool EvaluateVolumeObstacle(AActor* Target, UTopDown2DShapeComp* ShapeComp);

    // --- Visibility reporting --- //
    void ReportVisibility(AActor* Target, bool bVisible);
    void ReportVisibilityIfChanged(AActor* Target, bool bVisible);

    // --- Helpers --- //
    UVision_VisualComp* GetVisualComp(AActor* Target) const;
    bool ShouldRunServerLogic() const;
    void StartEvaluationTimer();
    void StopEvaluationTimer();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Evaluator")
    USphereComponent* DetectionSphere = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Evaluator")
    TEnumAsByte<ECollisionChannel> VisionTargetChannel = ECC_GameTraceChannel1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Evaluator")
    float DetectionRadius = 1200.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Evaluator")
    float OcclusionThreshold = 0.9f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Evaluator")
    TEnumAsByte<ECollisionChannel> WallTraceChannel = ECC_Visibility;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Evaluator", meta=(ClampMin="0.01"))
    float EvaluationInterval = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Evaluator")
    FName TargetTag = TEXT("VisionTarget");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Evaluator|Debug")
    bool bDrawDebug = false;

private:
    UPROPERTY(Transient)
    TSet<AActor*> OverlappingTargets;

    FTimerHandle EvaluationTimerHandle;

    UPROPERTY(Transient)
    UVision_VisualComp* CachedVisualComp = nullptr;

    UPROPERTY(Transient)
    TMap<AActor*, bool> LastReportedVisibility;
};