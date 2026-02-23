// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstance.h"
#include "LineOfSight/VisionData.h"// now as enum
#include "CameraVisionManager.generated.h"


// Forward declaration
class ULineOfSightComponent;// for the local LOS stamps

/*
 * This is the Main RT which will Layer the LOS stamps which are visible, exist in camera vision range.
 */

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UCameraVisionManager : public UActorComponent
{
	GENERATED_BODY()
public:
	UCameraVisionManager();

protected:
	virtual void BeginPlay() override;
	
public:
	
	UFUNCTION(BlueprintCallable, Category="LineOfSight")// Initialize with the owning camera/player
	void Initialize();
	
	//Update Main CRT (called every frame or when dirty)
	UFUNCTION(BlueprintCallable, Category="LineOfSight")
	void UpdateCameraLOS();
	
	/** Get the current camera-local RT for post-process */
	UCanvasRenderTarget2D* GetCameraLOSTexture() const { return CameraLocalRT; }

	UFUNCTION(BlueprintCallable, Category="LineOfSight")
	UMaterialInstanceDynamic* GetLayeredMID() const {return LayeredLOSInterfaceMID;}
	
private:
	//Internal helpers
	
	/** Actually draws all registered LOS providers to the Canvas */
	UFUNCTION()
	void DrawLOS_CPU(UCanvas* Canvas, int32 Width, int32 Height);
	
	//Helper function for drawing LOS Stamps
	void DrawLOSStamp(
		UCanvas* Canvas,
		const TArray<ULineOfSightComponent*>& Providers,
		const FLinearColor& Color);
	//This uses CPU method. Every LOS Stamps use its own DrawCall and it is causing bottle neck. need better solution

	//GPU method. Pass the struct to the material and layer them in there. only need one draw call.
	void RenderLOS_GPU(
		FRDGBuilder& GraphBuilder,
		FRDGTextureRef LOSTexture);
	
	
	bool ConvertWorldToRT(// this will output the relative coord to be used for pivot of the LOS stamps
		const FVector& ProviderWorldLocation,
		const float& ProviderVisionRange,
		//out
		FVector2D& OutPixelPosition,
		float& OutTileSize) const;

	bool GetVisibleProviders(
		//Out
		TArray<ULineOfSightComponent*>& OutProviders) const;

	bool ShouldRunClientLogic() const;// can it run as client or not



	
protected:
	//CPU or GPU
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision")
	bool bUseCPU=true;
	
	//Debug draw
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision")
	bool bDrawTextureRange =false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision")
	EVisionChannel VisionChannel=EVisionChannel::None;
	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision")
	float CameraVisionRange;// the half-radius of the camera view range
	// or just a CRT texture pixel size
	
	/** Camera-local render target */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision")
	UCanvasRenderTarget2D* CameraLocalRT = nullptr;

	//MPC
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision")
	UMaterialParameterCollection* PostProcessMPC=nullptr;// update MPC not MID. the post process is just one
	UPROPERTY()
	UMaterialParameterCollectionInstance* MPCInstance=nullptr;
	
	//MPC Param for PostProcess Material
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MPC")
	FName MPCLocationParam=TEXT("VisionCenterLocation");
	
	/*UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MPC")// tuned out, the material doesn't need to know the resoultion. just size is enough
	FName MPCTextureSizeParam=NAME_None;*/
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MPC")
	FName MPCVisibleRangeParam=TEXT("VisibleRange");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MPC")
	FName MPCNearSightRangeParam=TEXT("NearSightRange");
	
	/** Material used for stamping LOS sources */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision")
	UMaterialInterface* LOSMaterial=nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vision")
	UMaterialInstanceDynamic* LOSMaterialInstance = nullptr;


	/** Interface material that exposes CameraLocalRT as a texture */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision")
	UMaterialInterface* LayeredLOSInterfaceMaterial = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vision")
	UMaterialInstanceDynamic* LayeredLOSInterfaceMID = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision")
	FName LayeredLOSTextureParam = TEXT("RenderTarget");


	//Gpu method requirement
	UPROPERTY(EditAnywhere, Category="Vision")
	uint32 CameraViewChannelMask = 0xFFFFFFFF;
	
	/*
	/** Resolution of the camera-local RT #1#
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision")
	int32 RTSize = 1024;
	//-*///--> the CRT is already made in the Content browser, so no need to have resoultion in here

	static uint32 MakeChannelBitMask(const TArray<EVisionChannel>& ChannelEnums);

};
