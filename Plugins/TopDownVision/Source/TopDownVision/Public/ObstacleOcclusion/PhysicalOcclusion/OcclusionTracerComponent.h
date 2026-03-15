#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "TopDownVision/Public/ObstacleOcclusion/PhysicalOcclusion/OcclusionTraceTypes.h"
#include "TopDownVision/Public/ObstacleOcclusion/PhysicalOcclusion/FrustumToProjectionMatcherHelper.h"
#include "OcclusionTracerComponent.generated.h"

class UCameraComponent;
class APlayerCameraManager;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UOcclusionTracerComponent : public USceneComponent
{
    GENERATED_BODY()

public:

    UOcclusionTracerComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintCallable, Category="Occlusion Tracer")
    void InitializeProbeTracer();

    UFUNCTION(BlueprintCallable, Category="Occlusion Tracer")
    void SetCameraComponent(UCameraComponent* InCamera);

    UFUNCTION(BlueprintCallable, Category="Occlusion Tracer")
    void SetCameraManager(APlayerCameraManager* InCameraManager);

    UFUNCTION(BlueprintCallable, Category="Occlusion Tracer")
    void OnOwnerBecameVisible();

    UFUNCTION(BlueprintCallable, Category="Occlusion Tracer")
    void OnOwnerBecameHidden();

    UFUNCTION(BlueprintNativeEvent, Category="Occlusion Tracer")
    void OnTracerActivated();

    UFUNCTION(BlueprintNativeEvent, Category="Occlusion Tracer")
    void OnTracerDeactivated();

    // ── Config ────────────────────────────────────────────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer")
    TArray<FOcclusionSweepConfig> SweepConfigs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer")
    float TargetVisibleRadius = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer")
    FVector TargetLocationOffset = FVector(0.f, 0.f, 100.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer")
    TEnumAsByte<ECollisionChannel> OcclusionChannel = ECC_GameTraceChannel1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer")
    float TraceInterval = 0.1f;

    // ── Sweep generation ──────────────────────────────────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer|Sweeps")
    bool bAutoGenerateSweeps = true;

    /** Hard cap on generated sweep count */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer|Sweeps",
              meta=(EditCondition="bAutoGenerateSweeps", ClampMin=1))
    int32 MaxAutoSweepCount = 20;

    /** Distance between each sphere = previous sphere radius * this value.
     *  1.0 = spheres touch, 2.0 = one radius gap between each */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer|Sweeps",
              meta=(EditCondition="bAutoGenerateSweeps", ClampMin=0.1f))
    float SweepGapRatio = 1.5f;

    // ── Rocket head ───────────────────────────────────────────────────────

    // World units from target where rocket head taper takes effect
    // Beyond this distance — normal frustum matching is used
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer|Sweeps|RocketHead",
              meta=(ClampMin=0.f))
    float RocketHeadDistance = 150.f;

    // Controls taper shape within rocket head zone
    // 1.0 = linear, < 1.0 = gradual (convex), > 1.0 = steep (concave)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer|Sweeps|RocketHead",
              meta=(ClampMin=0.1f))
    float RocketHeadExponent = 2.f;

    // ── Debug ─────────────────────────────────────────────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer|Debug")
    bool bDebugDraw = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer|Debug")
    FColor DebugColorNoHit = FColor::Green;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer|Debug")
    FColor DebugColorHit = FColor::Red;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer|Debug")
    FColor DebugColorTarget = FColor::Cyan;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer|Debug")
    FColor DebugColorSweepOrigin = FColor::Yellow;

private:

    void RunTrace();
    void RebuildProbe();
    bool RefreshFrustumParams();
    void DrawDebug() const;

    // Can be overridden — e.g. curved world shader needs curved placement
    virtual void GenerateSweepsAlongLine(const FVector& LineDirection, float LineLength);

    FOcclusionProbe Probe;
    FCameraFrustumParams FrustumParams;

    UPROPERTY()
    TArray<AActor*> IgnoredActors;

    UPROPERTY()
    TObjectPtr<UCameraComponent> CameraComponent = nullptr;

    UPROPERTY()
    TObjectPtr<APlayerCameraManager> CameraManager = nullptr;

    FTimerHandle TraceTimerHandle;

    bool bCameraReady = false;
};