#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TopDownVision/Public/ObstacleOcclusion/PhysicalOcclusion/FrustumToProjectionMatcherHelper.h"
#include "MainOcclusionPainter.generated.h"

class UTextureRenderTarget2D;
class APlayerController;

TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(OcclusionPainter, Log, All);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UMainOcclusionPainter : public UActorComponent
{
    GENERATED_BODY()

public:

    UMainOcclusionPainter();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // ── Update — called externally by TopDownCameraComp tick ─────────────

    UFUNCTION(BlueprintCallable, Category="OcclusionPainter")
    void InitializeOcclusionComponent(APlayerController* InPC);

    UFUNCTION(BlueprintCallable, Category="OcclusionPainter")
    void UpdateOcclusionRT();

    // ── Camera source ─────────────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category="OcclusionPainter")
    void SetPlayerController(APlayerController* InPC);

    // ── Config ────────────────────────────────────────────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OcclusionPainter")
    TObjectPtr<UTextureRenderTarget2D> OcclusionRT;

    // Default brush material — used when target has no per-target material set
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OcclusionPainter")
    TObjectPtr<UMaterialInterface> DefaultBrushMaterial;

    // Scalar — scales brush intensity per target (0-1)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OcclusionPainter")
    FName RevealAlphaParam = TEXT("RevealAlpha");

private:

    void DrawProviderArea();
    bool RefreshFrustumParams();

    bool ShouldRunClientLogic();

    UPROPERTY(Transient)
    TObjectPtr<APlayerController> PlayerController;

    FCameraFrustumParams FrustumParams;

    bool bReady = false;
};