#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LineOfSight/VisionData.h"
#include "Vision_VisualComp.generated.h"

#pragma region Forward Declarations
class ULineOfSightComponent;
class ULOSObstacleDrawerComponent;
class ULOSStampDrawerComp;
class UMaterialInstanceDynamic;
class UVisibilityMeshComp;
class UTopDown2DShapeComp;
class UVision_EvaluatorComp;
struct FLOSStampPoolSlot;
#pragma endregion

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOcclusionTracerEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnVisibilityFadeComplete);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UVision_VisualComp : public UActorComponent
{
    GENERATED_BODY()

public:
    UVision_VisualComp();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void OnRegister() override;

public:

    UFUNCTION(BlueprintCallable, Category="Vision")
    void Initialize();

    UFUNCTION(BlueprintCallable, Category="Vision")
    void SetIndicatorRange(float NewIndicatorRange);

    UFUNCTION(BlueprintCallable, Category="Vision")
    float GetIndicatorRange() const { return IndicatorRange; }

    UFUNCTION(BlueprintCallable, Category="Vision")
    void UpdateVision();

    void ToggleLOSStampUpdate(bool bIsOn);

    UFUNCTION(BlueprintCallable, Category="Vision")
    bool IsUpdating() const;

    UFUNCTION(BlueprintCallable, Category="Vision")
    void SetVisible(bool bVisible, bool bInstant = false);

    UFUNCTION(BlueprintCallable, Category="Vision")
    float GetVisibilityAlpha() const { return VisibilityAlpha; }

    UFUNCTION(BlueprintCallable, Category="Vision")
    void SetVisionRange(float NewRange);

    float GetVisibleRange()    const { return VisionRange; }
    float GetMaxVisibleRange() const { return MaxVisionRange; }

    UFUNCTION(BlueprintCallable, Category="Vision")
    UMaterialInstanceDynamic* GetStampMID() const;

    UFUNCTION(BlueprintCallable, Category="Vision")
    ULOSObstacleDrawerComponent* GetObstacleDrawer()  const { return ObstacleDrawer; }

    UFUNCTION(BlueprintCallable, Category="Vision")
    ULOSStampDrawerComp* GetStampDrawer()             const { return StampDrawer; }

    UFUNCTION(BlueprintCallable, Category="Vision")
    UVisibilityMeshComp* GetVisibilityMeshComp()      const { return VisibilityMesh; }

    UFUNCTION(BlueprintCallable, Category="Vision")
    UTopDown2DShapeComp* GetShapeComp()               const { return ShapeComp; }

    UFUNCTION(BlueprintCallable, Category="Vision")
    EVisionChannel GetVisionChannel() const { return VisionChannel; }

    UFUNCTION(BlueprintCallable, Category="Vision")
    void SetVisionChannel(EVisionChannel InVC);

    UFUNCTION(BlueprintCallable, Category="Vision")
    void UpdateVisionRange(float NewRange);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="Vision")
    bool IsSharedVisionChannel() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="Vision")
    EVisionChannel GetLocalPlayerVisionChannel() const;

    // ── Pool ─────────────────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category="Vision|Pool")
    void OnRevealed_EnterPool();

    UFUNCTION(BlueprintCallable, Category="Vision|Pool")
    void OnHidden_ExitPool();

    void OnPoolSlotAcquired(const FLOSStampPoolSlot& Slot);
    void OnPoolSlotReleased();

    bool UsesResourcePool() const { return bUseResourcePool; }
    bool HasPoolSlot()      const { return bHasActivePoolSlot; }

public:

    UPROPERTY(BlueprintAssignable, Category="Occlusion Tracer")
    FOcclusionTracerEvent OnTargetRevealed;

    UPROPERTY(BlueprintAssignable, Category="Occlusion Tracer")
    FOcclusionTracerEvent OnTargetHidden;

    UPROPERTY(BlueprintAssignable, Category="Vision")
    FOnVisibilityFadeComplete OnTargetRevealComplete;

    UPROPERTY(BlueprintAssignable, Category="Vision")
    FOnVisibilityFadeComplete OnTargetHideComplete;

#pragma region Components
private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vision", meta=(AllowPrivateAccess="true"))
    ULOSObstacleDrawerComponent* ObstacleDrawer = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vision", meta=(AllowPrivateAccess="true"))
    ULOSStampDrawerComp* StampDrawer = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vision", meta=(AllowPrivateAccess="true"))
    UVisibilityMeshComp* VisibilityMesh = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vision", meta=(AllowPrivateAccess="true"))
    UTopDown2DShapeComp* ShapeComp = nullptr;
#pragma endregion

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision", meta=(AllowPrivateAccess="true"))
    EVisionChannel VisionChannel = EVisionChannel::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision", meta=(AllowPrivateAccess="true"))
    float VisionRange = 800.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision", meta=(AllowPrivateAccess="true"))
    float MaxVisionRange = 800.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision", meta=(AllowPrivateAccess="true"))
    float FadeTickInterval = 0.033333f;

    UPROPERTY(EditAnywhere, Category="Vision", meta=(AllowPrivateAccess="true"))
    float FadeSpeed = 5.0f;

    float VisibilityAlpha = 0.0f;
    float TargetVisibilityAlpha = 0.0f;
    FTimerHandle FadeTimerHandle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision", meta=(AllowPrivateAccess="true"))
    float ObstacleRedrawThreshold = 50.f;

    FVector LastObstacleDrawLocation = FVector(FLT_MAX);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision", meta=(AllowPrivateAccess="true"))
    float IndicatorRange = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision|Pool", meta=(AllowPrivateAccess="true"))
    bool bUseResourcePool = true;

    bool bHasActivePoolSlot = false;

private:
    UPROPERTY(Transient)
    UVision_EvaluatorComp* CachedEvaluatorComp = nullptr;

    int32 OcclusionTargetIndex = INDEX_NONE;

private:
    bool ShouldRunClientLogic() const;
    void UpdateVisibilityFade(float DeltaTime);
};
