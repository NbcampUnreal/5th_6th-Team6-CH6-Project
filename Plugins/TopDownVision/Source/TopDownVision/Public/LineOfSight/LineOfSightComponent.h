#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/CanvasRenderTarget2D.h"// for layer composition
#include "Materials/MaterialInterface.h"
//#include "Materials/MaterialParameterCollection.h"// to update the location -> for dynamic number of the stamps, this is done by MID
#include "Materials/MaterialInstanceDynamic.h"
#include "LineOfSightComponent.generated.h"


//LOG

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API ULineOfSightComponent : public UActorComponent
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

    UFUNCTION(BlueprintCallable, Category="LineOfSight")
    void UpdateMID();
    
    //Getter for the RT
    UCanvasRenderTarget2D* GetLocalLOSTexture() const { return CanvasRenderTarget; }
    //getter for the LOS_MID
    UMaterialInstanceDynamic* GetLOSMaterialMID() const { return LOSMaterialMID; }

    //Vision Getter, Setter
    float GetVisibleRange() const {return VisionRange;}
    //Getter for MID Param Names
    void GetMIDParamNames(FName& OutLocationParam, FName& OutVisibleRangeParam) const
    {
        OutLocationParam=LocationVectorValueName;
        OutVisibleRangeParam=VisibleRangeScalarValueName;
    }

    //Switch function for update
    void ToggleUpdate(bool bIsOn);
    bool IsUpdating() const{return ShouldUpdate;}

    
protected:
    /** Called by CanvasRenderTarget to draw material */
    UFUNCTION()
    void DrawLOS(UCanvas* Canvas, int32 Width, int32 Height);

    void PrepareDynamics();// make CRT and MID

protected:
    /** Vision range (optional for material logic) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    float VisionRange = 800.f;

    /** Local render target for this actor */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    UCanvasRenderTarget2D* CanvasRenderTarget;
    //Rendertarget value

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    int32 PixelResolution=256;
    
    /** Material used to generate LOS mask */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    UMaterialInterface* LOSMaterial;

    UPROPERTY(Transient)// mark as transient, causee it does not have to be serialized and saved. runtime only
    UMaterialInstanceDynamic* LOSMaterialMID = nullptr;
    
    /*/** Material Parameter Collection for actor location #1#
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")//---> no more MPC
    UMaterialParameterCollection* VisionMPC;*/

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    FName LocationVectorValueName="VisionCenterLocation";
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LineOfSight")
    FName VisibleRangeScalarValueName="SightRange";

private:
    /** Internal dirty flag to skip unnecessary updates */
    bool bDirty = true;

    bool ShouldUpdate=false;// only update when the camera vision capturing it
};
