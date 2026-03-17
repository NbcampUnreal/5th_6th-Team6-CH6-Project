#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "TopDownVision/Public/ObstacleOcclusion/OcclusionTracer/OcclusionTraceTypes.h"
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
     *  19 = solid coverage, 37 = near-perfect, 50 = maximum */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer|Fibonacci Cone",
              meta=(EditCondition="TraceMethod==EOcclusionTraceMethod::FibonacciCone", ClampMin=1, ClampMax=50))
    int32 NumTracePoints = 19;

    // ── Sphere array config ───────────────────────────────────────────────

    /** Hard cap on generated sweep count */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer|Sphere Array",
              meta=(EditCondition="TraceMethod==EOcclusionTraceMethod::SphereArray", ClampMin=1))
    int32 MaxAutoSweepCount = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer|Sphere Array",
			  meta=(EditCondition="TraceMethod==EOcclusionTraceMethod::SphereArray", ClampMin=1))
	float SweepGapRatio = 1.5;
	
    /** Minimum distance from target before placing first sphere —
     *  skips the area immediately adjacent to the pawn where false positives occur */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer|Sphere Array",
              meta=(EditCondition="TraceMethod==EOcclusionTraceMethod::SphereArray", ClampMin=0.f))
    float MinDistFromTarget = 100.f;

    /** Maximum distance from camera to place spheres —
     *  occluders beyond this range cannot realistically cover the pawn on screen */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer|Sphere Array",
              meta=(EditCondition="TraceMethod==EOcclusionTraceMethod::SphereArray", ClampMin=0.f))
    float MaxDistFromCamera = 2000.f;

    // ── Rocket head (sphere array only) ───────────────────────────────────

    // World units from target where rocket head taper takes effect
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer|Sphere Array|Rocket Head",
              meta=(EditCondition="TraceMethod==EOcclusionTraceMethod::SphereArray", ClampMin=0.f))
    float RocketHeadDistance = 150.f;

    // Controls taper shape within rocket head zone
    // 1.0 = linear, < 1.0 = gradual (convex), > 1.0 = steep (concave)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer|Sphere Array|Rocket Head",
              meta=(EditCondition="TraceMethod==EOcclusionTraceMethod::SphereArray", ClampMin=0.1f))
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

protected:

    // Rebuilds probe data each trace — override in curved world subclass
    virtual void RebuildProbe();

    // Sphere array generation — override in curved world subclass
    virtual void GenerateSweepsAlongLine(const FVector& LineDirection, float LineLength);

    // Fibonacci cone generation — override in curved world subclass
    virtual void GenerateFibonacciLines(const FVector& TargetLocation);

    FOcclusionProbe      Probe;
    FCameraFrustumParams FrustumParams;

    UPROPERTY()
    TArray<AActor*> IgnoredActors;

    UPROPERTY()
    TObjectPtr<UCameraComponent> CameraComponent = nullptr;

    UPROPERTY()
    TObjectPtr<APlayerCameraManager> CameraManager = nullptr;

private:

    void RunTrace();
    bool RefreshFrustumParams();
    virtual void DrawDebug() const;

    FTimerHandle TraceTimerHandle;
    bool bCameraReady = false;
};