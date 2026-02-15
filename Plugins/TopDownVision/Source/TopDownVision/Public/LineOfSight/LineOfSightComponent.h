#pragma once

#include "CoreMinimal.h"
//#include "Components/SceneComponent.h"
#include "Components/ActorComponent.h"
#include "LineOfSight/VisionData.h"// now as enum
#include "LineOfSightComponent.generated.h"


//forwardDeclare
class UTextureRenderTarget2D;// for locally capturing the environment data
class ULocalTextureSampler;// for sampling pre-baked texture into local RT

class UMaterialInterface;
class UMaterialInstanceDynamic;

// Vision helpers
class USphereComponent;
class UShapeAwareVisibilityTracer;
class UPrimitiveComponent;

class ULOSVisionSubsystem;
class UVolumeVisibilityEvaluatorComp;
class UVisionGameStateComp;


//LOG

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API ULineOfSightComponent : public UActorComponent//changed it into SceneComp for 2dSceneComp can be attached to
{
    GENERATED_BODY()

public:
    ULineOfSightComponent();

protected:
    virtual void BeginPlay() override;

public:
    UFUNCTION(BlueprintCallable, Category="LineOfSight")
    void UpdateLocalLOS();//LOS stamp

    UFUNCTION(BlueprintCallable, Category="LineOfSight")
    void UpdateTargetDetection();// this is for target detection update.
    //-> updating it per tick seems bit too much. use different timer
    
    UFUNCTION(BlueprintCallable, Category="LineOfSight")
    void UpdateVisibleRange(float NewRange);// this only updates when range change
    //no need for the location
    
    //Getter for the RT
    UTextureRenderTarget2D* GetLocalLOSTexture() const { return LOSRenderTarget; }
    //getter for the LOS_MID
    UFUNCTION(BlueprintCallable, Category="LineOfSight")
    UMaterialInstanceDynamic* GetLOSMaterialMID() const { return LOSMaterialMID; }

    //Vision Getter, Setter
    float GetVisibleRange() const {return VisionRange;}
    float GetMaxVisibleRange() const {return MaxVisionRange;}
    
    //Getter for Channel
    UFUNCTION(BlueprintCallable, Category="LineOfSight")
    EVisionChannel GetVisionChannel()const {return VisionChannel;}
    UFUNCTION(BlueprintCallable, Category="LineOfSight")
    void SetVisionChannel(EVisionChannel NewChannel) {VisionChannel = NewChannel;}
    
    //Switch function for update
    void ToggleLOSStampUpdate(bool bIsOn);
    bool IsUpdating() const{return ShouldUpdateLOSStamp;}

    
protected:
    //========== Physical Detection =========//
    UFUNCTION()
    void OnVisionSphereBeginOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

    UFUNCTION()
    void OnVisionSphereEndOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex);

    
private:
    //prep
    void CreateResources();// make RT and MID

    bool ShouldRunClientLogic()const;
    
    /** Resolve which component represents the actor’s visibility shape */
    UPrimitiveComponent* ResolveVisibilityShape(AActor* TargetActor) const;

   void HandleTargetVisibilityChanged(AActor* DetectedTarget, bool bIsVisible);// this will be used for updating the visible actor
    
    //Helper for getting Vision GameStateComp from the Gamestate
    UVisionGameStateComp* GetVisionGameStateComp();

    //Helper for getting LOSVisionSubsystem
    ULOSVisionSubsystem* GetLOSVisionSubsystem();

    //UVisibilityTargetComp* GetVisibilityTargetComp(AActor* OwnerActor);

    //void OnVisibleStateChanged(bool bIsVisible);// this will trigger the VisibilityAlpha update using lerp
    
protected:
    
    //Debug for toggling activation
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    bool bDrawTextureRange = false;
    
    //Vision Channel of this LOS stamp
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    EVisionChannel VisionChannel=EVisionChannel::None;//not registered yet
    /*
     *  0 for shared vision
     *  others are shared only by same channel
     */
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    float VisibilityAlpha=1.0f;//0~1
    //--> update the visibility alpha for update for appear and disappear anim
    
    /** Vision range (optional for material logic) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    float VisionRange = 800.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    float MaxVisionRange = 800.f;//<- this will be used for Fixed RenderTarget Size and OrthoWidth
    
    //WorldEnvironmentCapturing
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LineOfSight")
    ULocalTextureSampler* LocalTextureSampler;// now a default object
    
    // will be dynamically generated for the local LOS stamp, and be rendered by 2D Scene capture component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LineOfSight")
    UTextureRenderTarget2D* LOSRenderTarget;// this is for capturing 
    
    //RenderTarget value
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    int32 PixelResolution=256;
    
    /** Material used to generate LOS mask */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    UMaterialInterface* LOSMaterial=nullptr;

    UPROPERTY(Transient)// mark as transient, causee it does not have to be serialized and saved. runtime only
    UMaterialInstanceDynamic* LOSMaterialMID = nullptr;

    //MID Param
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    FName MIDTextureParam=NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    FName MIDVisibleRangeParam=NAME_None;

    //============= Physical Detection ==========//
#pragma region Detection
    //----------------------------------------------------------------------------------------------------------------//

    //Debug
    UPROPERTY(EditAnywhere, Category="LineOfSight|Detection")
    bool bDrawDetectionDebug = true;

    UPROPERTY(EditAnywhere, Category="LineOfSight|Detection")
    bool bDetectionEnabled = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LineOfSight|Detection")
    USphereComponent* VisionSphere = nullptr;

    UPROPERTY(Transient)
    UShapeAwareVisibilityTracer* VisibilityTracer = nullptr;// this will be used for doing shape aware tracing

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight|Detection")
    TEnumAsByte<ECollisionChannel> ObstacleTraceChannel = ECC_Visibility;
    //channel for the object tracing. use custom trace channel here
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight|Detection")
    float DesiredAngleDegree=5.f;
    
    //Tag for the actor to be targeted
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight|Detection")
    FName VisionTargetTag = TEXT("VisionTarget");

    //detection state record
    UPROPERTY(Transient)
    TMap<AActor*, bool> TargetVisibilityMap;//targets detected

private:
    UPROPERTY(Transient)//cached statecomp
    UVisionGameStateComp* CachedVisionGameStateComp = nullptr;
    
    //----------------------------------------------------------------------------------------------------------------//
#pragma endregion
    
private:
    bool ShouldUpdateLOSStamp=false;// only update when the camera vision capturing it
};
