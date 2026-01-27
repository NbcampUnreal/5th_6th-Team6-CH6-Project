// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/CanvasRenderTarget2D.h"// like the name, the canvas for compositing local RTs
#include "VisionSubsystem.generated.h"


//forward declare
class ULineOfSightComponent;// the source of the texture


/**
 *  this will be used for minimap. the main RT will be camera focused
 */
//Log
TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(VisionSubsystem, Log, All);

UCLASS()
class TOPDOWNVISION_API UVisionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	void RegisterProvider(ULineOfSightComponent* Provider);
	void UnregisterProvider(ULineOfSightComponent* Provider);

	//void UpdateGlobalLOS();// update the layered RT
	//Getter
	UCanvasRenderTarget2D* GetGlobalRenderTarget() const { return GlobalRenderTarget; }
private:
	// Draw callback for global canvas
	//UFUNCTION()
	//void DrawGlobalLOS(UCanvas* Canvas, int32 Width, int32 Height);
	
private:
	// Global composited render target
	UPROPERTY()
	UCanvasRenderTarget2D* GlobalRenderTarget = nullptr;

	// Registered actor-local LOS providers
	UPROPERTY()
	TArray<TObjectPtr<ULineOfSightComponent>> RegisteredProviders;

	// Resolution of global RT
	UPROPERTY(EditAnywhere, Category="Vision")
	int32 GlobalResolution = 1024;//default
};
