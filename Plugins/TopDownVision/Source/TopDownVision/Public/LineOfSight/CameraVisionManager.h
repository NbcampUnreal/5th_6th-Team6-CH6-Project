// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Materials/MaterialInterface.h"
#include "CameraVisionManager.generated.h"

// Forward declaration
class ULineOfSightComponent;// for the local LOS stamps

/*
 * This is the Main RT which will Layer the LOS stamps which are visible, exist in camera vision range.
 */

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UCameraVisionManager : public UObject
{
	GENERATED_BODY()

public:
	/** Initialize with the owning camera/player */
	void Initialize(APlayerCameraManager* InCamera);

	/** Register a LOS source (player, NPC, event) */
	void RegisterLOSProvider(ULineOfSightComponent* Provider);

	/** Unregister a LOS source */
	void UnregisterLOSProvider(ULineOfSightComponent* Provider);

	/** Update RT (called every frame or when dirty) */
	void UpdateCameraLOS();

	void SetActiveCamera(APlayerCameraManager* InCamera);

	
	/** Get the current camera-local RT for post-process */
	UCanvasRenderTarget2D* GetCameraLOSTexture() const { return CameraLocalRT; }

protected:
	/** Actually draws all registered LOS providers to the Canvas */
	UFUNCTION()
	void DrawLOS(UCanvas* Canvas, int32 Width, int32 Height);

protected:
	/** Owning camera */
	UPROPERTY()
	APlayerCameraManager* ActiveCamera = nullptr;

	/** Camera-local render target */
	UPROPERTY()
	UCanvasRenderTarget2D* CameraLocalRT = nullptr;

	/** Material used for stamping LOS sources */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision")
	UMaterialInterface* LOSMaterial;

	/** Resolution of the camera-local RT */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision")
	int32 RTSize = 1024;

	/** Registered LOS sources */
	UPROPERTY()
	TArray<TObjectPtr<ULineOfSightComponent>> LOSProviders;

	/** Dirty flag to avoid unnecessary redraws */
	bool bDirty = true;
};
