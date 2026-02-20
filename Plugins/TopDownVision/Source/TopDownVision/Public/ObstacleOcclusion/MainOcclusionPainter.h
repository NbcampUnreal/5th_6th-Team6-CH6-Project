// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MainOcclusionPainter.generated.h"

/*
 * When there are multiple occluder in the scene, it needs more than one area for masking
 *
 * this one draws the radial area on projected coord of occluder
 *
 * get provider from the LOSVisionSubsystem and draw brush on the projected coord or target
 *
 *
 * 
 * 
 */

//ForwardDeclares
class UTextureRenderTarget2D;// for main RT.

class ULineOfSightComponent;

//Log
TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(OcclusionPainter, Log, All);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UMainOcclusionPainter : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UMainOcclusionPainter();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:

	UFUNCTION(BlueprintCallable, Category = "OcclusionPainter")
	void UpdateOcclusionRT();

protected:

	void DrawProviderArea();// helper function for drawing masking area for the 

	
protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OcclusionPainter")
	UTextureRenderTarget2D* OcclusionRT;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OcclusionPainter")
	UMaterialInterface* MaskingMateiral;// this is for the radial masking material used for brush.
	// will use material
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="OcclusionPainter")
	UMaterialInstanceDynamic* MaskingMateiralMID;
};
