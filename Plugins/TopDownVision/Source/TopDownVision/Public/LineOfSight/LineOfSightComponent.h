#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "LineOfSight/VisionData.h"// now as enum
#include "LineOfSightComponent.generated.h"

//forwardDeclare
class UTextureRenderTarget2D;
class UMaterialInterface;
class UMaterialInstanceDynamic;

//LOG

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API ULineOfSightComponent : public USceneComponent//changed it into SceneComp for 2dSceneComp can be attached to
{
    GENERATED_BODY()

public:
    ULineOfSightComponent();

protected:
    virtual void BeginPlay() override;

public:
    UFUNCTION(BlueprintCallable, Category="LineOfSight")
    void UpdateLocalLOS();
    
    UFUNCTION(BlueprintCallable, Category="LineOfSight")
    void UpdateVisibleRange(float NewRange);// this only updates when range change
    //no need for the location
    
    //Getter for the RT
    UTextureRenderTarget2D* GetLocalLOSTexture() const { return HeightRenderTarget; }
    //getter for the LOS_MID
    UFUNCTION(BlueprintCallable, Category="LineOfSight")
    UMaterialInstanceDynamic* GetLOSMaterialMID() const { return LOSMaterialMID; }

    //Vision Getter, Setter
    float GetVisibleRange() const {return VisionRange;}

    //Getter for Channel
    UFUNCTION(BlueprintCallable, Category="LineOfSight")
    EVisionChannel GetVisionChannel()const {return VisionChannel;}
    UFUNCTION(BlueprintCallable, Category="LineOfSight")
    void SetVisionChannel(EVisionChannel NewChannel) {VisionChannel = NewChannel;}
    
    //Switch function for update
    void ToggleUpdate(bool bIsOn);
    bool IsUpdating() const{return ShouldUpdate;}


    // This will be used when Height map is captured locally, not sampling pre baked height
    //Obstacle registration
    /** Manually add an actor to the visibility list */
    UFUNCTION(BlueprintCallable, Category="LineOfSight")
    void RegisterObstacle(AActor* Obstacle);

    /** Scans the world for actors with a specific tag and adds them to the capture list */
    UFUNCTION(BlueprintCallable, Category="LineOfSight")
    void RefreshObstaclesByTag();

    
protected:
    //prep
    void CreateResources();// make CRT and MID

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
    
    /** Vision range (optional for material logic) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    float VisionRange = 800.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    float MaxVisionRange = 800.f;//<- this will be used for Fixed RenderTarget Size and OrthoWidth

    //EyeSight (Local Z height value for eye sight)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    float EyeSightHeight = 200.f;//about 2m?
    
    //Depth Capture
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    USceneCaptureComponent2D* SceneCaptureComp = nullptr;
    
    // will be dynamically generated for the local LOS stamp, and be rendered by 2D Scene capture component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LineOfSight")
    UTextureRenderTarget2D* HeightRenderTarget;// this is for capturing 
    
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
    FName MIDEyeSightHeightParam=NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    FName MIDVisibleRangeParam=NAME_None;
    
    /*/** Material Parameter Collection for actor location #1#
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")//---> no more MPC
    UMaterialParameterCollection* VisionMPC;*/

    
    //no longer need MPC, it is done by LayerCompositing Comp, CameraVisionManager. just need the TextureParam name
    /*UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    FName LocationVectorValueName="VisionCenterLocation";
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    FName VisibleRangeScalarValueName="SightRange";*/

    //Obstacle
    /** The tag used to identify meshes that should block LOS (e.g., "LOS_Blocker") */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    FName BlockerTag = TEXT("LOS_Blocker");

private:
    bool ShouldUpdate=false;// only update when the camera vision capturing it
};
