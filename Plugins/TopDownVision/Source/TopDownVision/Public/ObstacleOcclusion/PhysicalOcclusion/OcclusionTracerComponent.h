#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "TopDownVision/Public/ObstacleOcclusion/PhysicalOcclusion/OcclusionTraceTypes.h"
#include "TopDownVision/Public/ObstacleOcclusion/PhysicalOcclusion/FrustumToProjectionMatcherHelper.h"
#include "OcclusionTracerComponent.generated.h"

class UCameraComponent;
class APlayerCameraManager;

UENUM(BlueprintType)
enum class EOcclusionTraceMethod : uint8
{
    // Sphere array along camera-to-target line — frustum-aware sizing, rocket head near target
    SphereArray     UMETA(DisplayName="Sphere Array"),

    // Fibonacci cone multi-line traces — simpler, no behind-target problem
    FibonacciCone   UMETA(DisplayName="Fibonacci Cone"),
};

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

    // Which trace method to use
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer")
    EOcclusionTraceMethod TraceMethod = EOcclusionTraceMethod::FibonacciCone;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer")
    float TargetVisibleRadius = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer")
    FVector TargetLocationOffset = FVector(0.f, 0.f, 100.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer")
    TEnumAsByte<ECollisionChannel> OcclusionChannel = ECC_GameTraceChannel1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer")
    float TraceInterval = 0.1f;

    // ── Fibonacci cone config ─────────────────────────────────────────────

    /** Number of line trace endpoints distributed via fibonacci pattern.
     *  19 = solid coverage, 37 = near-perfect, higher = diminishing returns */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer|Fibonacci Cone",
              meta=(EditCondition="TraceMethod==EOcclusionTraceMethod::FibonacciCone", ClampMin=1))
    int32 NumTracePoints = 19;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer|Fibonacci Cone",
		  meta=(EditCondition="TraceMethod==EOcclusionTraceMethod::FibonacciCone", ClampMin=0.f))
	float BehindTargetFilterOffset = 50.f;

    // ── Sphere array config (legacy) ──────────────────────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer|Sphere Array")
    TArray<FOcclusionSweepConfig> SweepConfigs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer|Sphere Array",
              meta=(EditCondition="TraceMethod==EOcclusionTraceMethod::SphereArray"))
    bool bAutoGenerateSweeps = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer|Sphere Array",
              meta=(EditCondition="bAutoGenerateSweeps", ClampMin=1))
    int32 MaxAutoSweepCount = 20;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer|Sphere Array",
              meta=(EditCondition="bAutoGenerateSweeps", ClampMin=0.1f))
    float SweepGapRatio = 1.5f;

    // ── Rocket head (sphere array only) ───────────────────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer|Sphere Array|Rocket Head",
              meta=(ClampMin=0.f))
    float RocketHeadDistance = 150.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer|Sphere Array|Rocket Head",
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

    // Sphere array generation — can be overridden for curved world
    virtual void GenerateSweepsAlongLine(const FVector& LineDirection, float LineLength);

    // Fibonacci cone generation
    void GenerateFibonacciLines(const FVector& TargetLocation);

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