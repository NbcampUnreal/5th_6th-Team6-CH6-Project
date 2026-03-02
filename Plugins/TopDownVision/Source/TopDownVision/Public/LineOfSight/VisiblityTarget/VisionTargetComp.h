#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LineOfSight/VisionData.h"//vision channel
#include "VisionTargetComp.generated.h"


#pragma region Forward Declarations
//ForwardDeclares
class ULineOfSightComponent;
class ULOSObstacleDrawerComponent;
class ULOSStampDrawerComp;
class UMaterialInstanceDynamic;
class UVisibilityMeshComp;




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
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UVisionTargetComp : public UActorComponent
{
    GENERATED_BODY()

public:
    UVisionTargetComp();

protected:
    virtual void BeginPlay() override;

public:
    // --- Called by the RT manager --- //

    /** Drive obstacle sampling and LOS stamp rendering for this frame. */
    UFUNCTION(BlueprintCallable, Category="Vision")
    void UpdateVision();

    /** Toggle whether the stamp should update this frame. */
    void ToggleLOSStampUpdate(bool bIsOn);
    bool IsUpdating() const;

    // --- Visibility fade --- //

    /** Trigger reveal or hide lerp. Called by the vision evaluator on detection change. */
    UFUNCTION(BlueprintCallable, Category="Vision")
    void SetVisible(bool bVisible);

    float GetVisibilityAlpha() const { return VisibilityAlpha; }

    // --- Range (source of truth) --- //

    UFUNCTION(BlueprintCallable, Category="Vision")
    void SetVisionRange(float NewRange);

    float GetVisibleRange()    const { return VisionRange; }
    float GetMaxVisibleRange() const { return MaxVisionRange; }

    // --- Pass-through getters for the RT manager --- //

    UFUNCTION(BlueprintCallable, Category="Vision")
    EVisionChannel GetVisionChannel() const {return VisionChannel;}

    /** The LOS stamp MID used by the RT manager to composite this unit's vision. */
    UFUNCTION(BlueprintCallable, Category="Vision")
    UMaterialInstanceDynamic* GetStampMID() const;

    // --- Subobject access --- //
    ULOSObstacleDrawerComponent* GetObstacleDrawer()  const { return ObstacleDrawer; }
    ULOSStampDrawerComp* GetStampDrawer()             const { return StampDrawer; }
    UVisibilityMeshComp* GetVisibilityMeshComp()           const { return VisibilityMesh; }

private:
    bool ShouldRunClientLogic() const;
    void UpdateVisibilityFade();

private:

#pragma region Components

    //obstacle RT drawer
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vision", meta=(AllowPrivateAccess="true"))
    ULOSObstacleDrawerComponent* ObstacleDrawer = nullptr;
    // LOS vision RT Drawer
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vision", meta=(AllowPrivateAccess="true"))
    ULOSStampDrawerComp* StampDrawer = nullptr;
    //Vision opacity updater
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vision", meta=(AllowPrivateAccess="true"))
    UVisibilityMeshComp* VisibilityMesh = nullptr;

#pragma endregion Components

    // --- Vision channel ---//
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision", meta=(AllowPrivateAccess="true"))
    EVisionChannel VisionChannel= EVisionChannel::None;//default none
    
    // --- Range --- //
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision", meta=(AllowPrivateAccess="true"))
    float VisionRange = 800.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision", meta=(AllowPrivateAccess="true"))
    float MaxVisionRange = 800.f;


    
    // --- Dynamic Fade --- //
    UPROPERTY(EditAnywhere, Category="Vision", meta=(AllowPrivateAccess="true"))
    float FadeSpeed = 5.0f;

    float VisibilityAlpha = 0.0f;
    float TargetVisibilityAlpha = 0.0f;
    FTimerHandle FadeTimerHandle;
};
