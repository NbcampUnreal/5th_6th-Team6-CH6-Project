#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Engine/CanvasRenderTarget2D.h"// for layer composition
#include "Materials/MaterialInterface.h"
//#include "Materials/MaterialParameterCollection.h"// to update the location -> for dynamic number of the stamps, this is done by MID
#include "Materials/MaterialInstanceDynamic.h"
#include "LineOfSightComponent.generated.h"


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
    UCanvasRenderTarget2D* GetLocalLOSTexture() const { return CanvasRenderTarget; }
    //getter for the LOS_MID
    UMaterialInstanceDynamic* GetLOSMaterialMID() const { return LOSMaterialMID; }

    //Vision Getter, Setter
    float GetVisibleRange() const {return VisionRange;}

    //Getter for Channel
    UFUNCTION(BlueprintCallable, Category="LineOfSight")
    int32 GetVisionChannel()const {return VisionChannel;}
    UFUNCTION(BlueprintCallable, Category="LineOfSight")
    void SetVisionChannel(int32 NewChannel) {VisionChannel = NewChannel;}
    
    //Switch function for update
    void ToggleUpdate(bool bIsOn);
    bool IsUpdating() const{return ShouldUpdate;}

    
protected:
    /** Called by CanvasRenderTarget to draw material */
    UFUNCTION()
    void DrawLOS(UCanvas* Canvas, int32 Width, int32 Height);

    void CreateResources();// make CRT and MID

protected:
    //Depth Capture
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    USceneCaptureComponent2D* SceneCaptureComp = nullptr;


    
    //Vision Channel of this LOS stamp
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    int32 VisionChannel=INDEX_NONE;//not registered yet
    /*
     *  0 for shared vision
     *  others are shared only by same channel
     */
    
    /** Vision range (optional for material logic) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    float VisionRange = 800.f;

    // will be dynamically generated for the local LOS stamp, and be rendered by 2D Scene capture component
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    UCanvasRenderTarget2D* CanvasRenderTarget;
    
    //Rendertarget value
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    int32 PixelResolution=256;
    
    /** Material used to generate LOS mask */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    UMaterialInterface* LOSMaterial=nullptr;

    UPROPERTY(Transient)// mark as transient, causee it does not have to be serialized and saved. runtime only
    UMaterialInstanceDynamic* LOSMaterialMID = nullptr;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    FName MIDTextureParam=NAME_None;
    
    /*/** Material Parameter Collection for actor location #1#
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")//---> no more MPC
    UMaterialParameterCollection* VisionMPC;*/

    
    //no longer need MPC, it is done by LayerCompositing Comp, CameraVisionManager. just need the TextureParam name
    /*UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    FName LocationVectorValueName="VisionCenterLocation";
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    FName VisibleRangeScalarValueName="SightRange";*/

private:
    bool ShouldUpdate=false;// only update when the camera vision capturing it
};
