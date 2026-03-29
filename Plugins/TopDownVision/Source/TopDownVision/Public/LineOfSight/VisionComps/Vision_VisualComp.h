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

#pragma endregion Forward Declarations

/**
 * Hub component for vision-related logic on a unit.
 *
 * Owns ObstacleDrawer, StampDrawer, VisibilityMesh as subobjects.
 * Holds vision range as the source of truth.
 * Manages the reveal/hide visibility fade via timer.
 * Registered with ULOSVisionSubsystem as the provider.
 * Updated externally by the vision RT manager.
 * Gates all client-only work behind ShouldRunClientLogic.
 *
 * Pool mode (bUseResourcePool = true):
 *   Resources (ObstacleRT, StampMID, VisibilityMesh MIDs) are NOT created at Initialize.
 *   ULOSRequirementPoolSubsystem hands them in via OnPoolSlotAcquired when the provider
 *   enters the camera's vision range, and reclaims them via OnPoolSlotReleased on exit.
 *
 * Owned mode (bUseResourcePool = false):
 *   Resources are created once at Initialize and kept for the actor's lifetime.
 *   Use this for always-visible units (e.g. the local player's own unit).
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOcclusionTracerEvent);

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

    // --- Called by the RT manager --- //

    UFUNCTION(BlueprintCallable, Category="Vision")
    void UpdateVision();

    void ToggleLOSStampUpdate(bool bIsOn);

    UFUNCTION(BlueprintCallable, Category="Vision")
    bool IsUpdating() const;

    // --- Visibility fade --- //

    UFUNCTION(BlueprintCallable, Category="Vision")
    void SetVisible(bool bVisible, bool bInstant = false);

    UFUNCTION(BlueprintCallable, Category="Vision")
    float GetVisibilityAlpha() const { return VisibilityAlpha; }

    // --- Range --- //

    UFUNCTION(BlueprintCallable, Category="Vision")
    void SetVisionRange(float NewRange);

    float GetVisibleRange()    const { return VisionRange; }
    float GetMaxVisibleRange() const { return MaxVisionRange; }

    // --- Stamp MID passthrough --- //

    UFUNCTION(BlueprintCallable, Category="Vision")
    UMaterialInstanceDynamic* GetStampMID() const;

    // --- Subobject access --- //

    UFUNCTION(BlueprintCallable, Category="Vision")
    ULOSObstacleDrawerComponent* GetObstacleDrawer()  const { return ObstacleDrawer; }

    UFUNCTION(BlueprintCallable, Category="Vision")
    ULOSStampDrawerComp* GetStampDrawer()             const { return StampDrawer; }

    UFUNCTION(BlueprintCallable, Category="Vision")
    UVisibilityMeshComp* GetVisibilityMeshComp()      const { return VisibilityMesh; }

    UFUNCTION(BlueprintCallable, Category="Vision")
    UTopDown2DShapeComp* GetShapeComp()               const { return ShapeComp; }

    // --- Vision channel --- //

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

    // ---------------------------------------------------------------- //
    //  Pool enter / exit
    //  Called exclusively by ULOSRequirementPoolSubsystem.
    //  Distributes slot resources to sub-components on acquire,
    //  strips them on release.
    // ---------------------------------------------------------------- //

    /** Called by the pool subsystem when a slot is assigned.
     *  Distributes ObstacleRT, StampMID, and VisibilityMesh MIDs to sub-components. */
    void OnPoolSlotAcquired(const FLOSStampPoolSlot& Slot);

    /** Called by the pool subsystem when the slot is reclaimed.
     *  Nulls out all pooled resources from sub-components. */
    void OnPoolSlotReleased();

    // --- Pool state queries --- //

    bool UsesResourcePool() const { return bUseResourcePool; }
    bool HasPoolSlot()      const { return bHasActivePoolSlot; }

public:

    UPROPERTY(BlueprintAssignable, Category="Occlusion Tracer")
    FOcclusionTracerEvent OnTargetRevealed;

    UPROPERTY(BlueprintAssignable, Category="Occlusion Tracer")
    FOcclusionTracerEvent OnTargetHidden;

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

#pragma endregion Components

    // --- Vision channel --- //
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision", meta=(AllowPrivateAccess="true"))
    EVisionChannel VisionChannel = EVisionChannel::None;

    // --- Range --- //
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision", meta=(AllowPrivateAccess="true"))
    float VisionRange = 800.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision", meta=(AllowPrivateAccess="true"))
    float MaxVisionRange = 800.f;

    // --- Dynamic Fade --- //
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision", meta=(AllowPrivateAccess="true"))
    float FadeTickInterval = 0.033333f;

    UPROPERTY(EditAnywhere, Category="Vision", meta=(AllowPrivateAccess="true"))
    float FadeSpeed = 5.0f;

    float VisibilityAlpha = 0.0f;
    float TargetVisibilityAlpha = 0.0f;
    FTimerHandle FadeTimerHandle;

    // --- Movement threshold --- //
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision", meta=(AllowPrivateAccess="true"))
    float ObstacleRedrawThreshold = 50.f;

    FVector LastObstacleDrawLocation = FVector(FLT_MAX);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision", meta=(AllowPrivateAccess="true"))
    float IndicatorRange = 0.f;

    // ---------------------------------------------------------------- //
    //  Pool
    // ---------------------------------------------------------------- //

    /** If true, resources (ObstacleRT, StampMID, VisibilityMesh MIDs) come from the pool.
     *  Acquire on enter range, release on exit.
     *  If false, resources are created once at Initialize and owned for lifetime. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision|Pool", meta=(AllowPrivateAccess="true"))
    bool bUseResourcePool = true;

    /** True while a pool slot is actively assigned to this provider.
     *  Set by OnPoolSlotAcquired / OnPoolSlotReleased. */
    bool bHasActivePoolSlot = false;

private:

    UPROPERTY(Transient)
    UVision_EvaluatorComp* CachedEvaluatorComp = nullptr;

    int32 OcclusionTargetIndex = INDEX_NONE;

private:
    bool ShouldRunClientLogic() const;
    void UpdateVisibilityFade(float DeltaTime);
};
