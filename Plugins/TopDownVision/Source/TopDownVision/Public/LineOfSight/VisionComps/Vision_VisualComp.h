#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LineOfSight/VisionData.h"//vision channel
#include "Vision_VisualComp.generated.h"


#pragma region Forward Declarations

//ForwardDeclares
class ULineOfSightComponent;
class ULOSObstacleDrawerComponent;
class ULOSStampDrawerComp;
class UMaterialInstanceDynamic;
class UVisibilityMeshComp;
class UTopDown2DShapeComp;
class UVision_EvaluatorComp;


#pragma endregion Forward Declarations

/**
 * Hub component for vision-related logic on a unit.
 *
 * Owns LOSComp, ObstacleDrawer, and StampDrawer as subobjects.
 * Holds vision range as the source of truth.
 * Manages the reveal/hide visibility fade via timer.
 * Registered with ULOSVisionSubsystem as the provider.
 * Updated externally by the vision RT manager.
 * Gates all client-only work behind ShouldRunClientLogic.
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

    /** Drive obstacle sampling and LOS stamp rendering for this frame. */
    UFUNCTION(BlueprintCallable, Category="Vision")
    void UpdateVision();

    /** Toggle whether the stamp should update this frame. */
    void ToggleLOSStampUpdate(bool bIsOn);
    UFUNCTION(BlueprintCallable, Category="Vision")
    bool IsUpdating() const;

    // --- Visibility fade --- //

    /** Trigger reveal or hide lerp. Called by the vision evaluator on detection change. */
    UFUNCTION(BlueprintCallable, Category="Vision")
    void SetVisible(bool bVisible, bool bInstant/* change instantly or not*/=false);
    
    UFUNCTION(BlueprintCallable, Category="Vision")
    float GetVisibilityAlpha() const { return VisibilityAlpha; }

    // --- Range (source of truth) --- //

    UFUNCTION(BlueprintCallable, Category="Vision")
    void SetVisionRange(float NewRange);

    float GetVisibleRange()    const { return VisionRange; }
    float GetMaxVisibleRange() const { return MaxVisionRange; }

    // --- Pass-through getters for the RT manager --- //


    /** The LOS stamp MID used by the RT manager to composite this unit's vision. */
    UFUNCTION(BlueprintCallable, Category="Vision")
    UMaterialInstanceDynamic* GetStampMID() const;

    // --- Subobject access --- //
    UFUNCTION(BlueprintCallable, Category="Vision")
    ULOSObstacleDrawerComponent* GetObstacleDrawer()  const { return ObstacleDrawer; }
    
    UFUNCTION(BlueprintCallable, Category="Vision")
    ULOSStampDrawerComp* GetStampDrawer()             const { return StampDrawer; }
    
    UFUNCTION(BlueprintCallable, Category="Vision")
    UVisibilityMeshComp* GetVisibilityMeshComp()           const { return VisibilityMesh; }
    
    UFUNCTION(BlueprintCallable, Category="Vision")
    UTopDown2DShapeComp*         GetShapeComp()         const { return ShapeComp; }


    //variable helper
    UFUNCTION(BlueprintCallable, Category="Vision")
    EVisionChannel GetVisionChannel() const {return VisionChannel;}
    UFUNCTION(BlueprintCallable, Category="Vision")
    void SetVisionChannel(EVisionChannel InVC);


    UFUNCTION(BlueprintCallable, Category="Vision")
    void UpdateVisionRange(float NewRange);

    //Bool getter for checking if the vision comp shares the vision channel of the locally played player
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="Vision")
    bool IsSharedVisionChannel() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="Vision")
    EVisionChannel GetLocalPlayerVisionChannel() const;
    
private:
    bool ShouldRunClientLogic() const;
    void UpdateVisibilityFade(float DeltaTime);




public:
    UPROPERTY(BlueprintAssignable, Category="Occlusion Tracer")
    FOcclusionTracerEvent OnTargetRevealed;

    UPROPERTY(BlueprintAssignable, Category="Occlusion Tracer")
    FOcclusionTracerEvent OnTargetHidden;

#pragma region Components
    
private:
    
    //obstacle RT drawer
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vision", meta=(AllowPrivateAccess="true"))
    ULOSObstacleDrawerComponent* ObstacleDrawer = nullptr;
    // LOS vision RT Drawer
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vision", meta=(AllowPrivateAccess="true"))
    ULOSStampDrawerComp* StampDrawer = nullptr;
    //Vision opacity updater
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vision", meta=(AllowPrivateAccess="true"))
    UVisibilityMeshComp* VisibilityMesh = nullptr;
    //2d Shape for sample points+Shape type for being used for evaluation by Vision_Evaluator
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vision", meta=(AllowPrivateAccess="true"))
    UTopDown2DShapeComp* ShapeComp = nullptr;

#pragma endregion Components

    // --- Vision channel ---//
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision", meta=(AllowPrivateAccess="true"))
    EVisionChannel VisionChannel= EVisionChannel::None;//default none
    
    // --- Range --- // !!!! 
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


    // --- Movement Threshold for the update filter --- //
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision", meta=(AllowPrivateAccess="true"))
    float ObstacleRedrawThreshold = 50.f;

    FVector LastObstacleDrawLocation = FVector(FLT_MAX);

private:
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision", meta=(AllowPrivateAccess="true"))
    float IndicatorRange = 0.f;

private:

    //Cached Evaluator comp
    UPROPERTY(Transient)
    UVision_EvaluatorComp* CachedEvaluatorComp = nullptr;

    // --- Occlusion --- //
    int32 OcclusionTargetIndex = INDEX_NONE;// the registered index
};
