#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "ObstacleOcclusion/PhysicallOcclusion/OcclusionTraceTypes.h"
#include "ObstacleOcclusion/PhysicallOcclusion/FrustumToProjectionMatcherHelper.h"
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

    // ── Camera source ─────────────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category="Occlusion Tracer")
    void SetCameraComponent(UCameraComponent* InCamera);

    UFUNCTION(BlueprintCallable, Category="Occlusion Tracer")
    void SetCameraManager(APlayerCameraManager* InCameraManager);

    // ── Visibility driven API — call these from BP or C++ ─────────────────

    UFUNCTION(BlueprintCallable, Category="Occlusion Tracer")
    void OnBecameVisible();

    UFUNCTION(BlueprintCallable, Category="Occlusion Tracer")
    void OnBecameHidden();

    // ── BP events — implement in Blueprint to react ───────────────────────

    UFUNCTION(BlueprintNativeEvent, Category="Occlusion Tracer")
    void OnTracerActivated();

    UFUNCTION(BlueprintNativeEvent, Category="Occlusion Tracer")
    void OnTracerDeactivated();

    // ── Config ────────────────────────────────────────────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer")
    TArray<FOcclusionSweepConfig> SweepConfigs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer")
    float TargetVisibleRadius = 60.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer")
    FVector TargetLocationOffset = FVector(0.f, 0.f, 60.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer")
    TEnumAsByte<ECollisionChannel> OcclusionChannel = ECC_GameTraceChannel1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Tracer")
    float TraceInterval = 0.1f;

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

    FOcclusionProbe      Probe;
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