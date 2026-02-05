// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"

#include "LocalTextureSampler.generated.h"

/*
 * This is Texture renderer, which samples the world texture into local texture
 *
 * it is for replacing a heavy 2dScene capture component. no more capturing per tick
 *
 * it gathers the overlapping pre-baked height texture and merge it into one local texture
 *
 * No tick-based capture
 * No collision or overlap queries
 * Pure AABB math + GPU projection
 * GPU-only merge using material passes
 * 
 * fuck yeah
 */


//Forward declares
class UTextureRenderTarget2D;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UVisionSubsystem;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API ULocalTextureSampler : public USceneComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	ULocalTextureSampler();

protected:
	virtual void BeginPlay() override;
	
public:

	/** Force update of the local texture (teleport, vision change, etc.) */
	UFUNCTION(BlueprintCallable, Category="LocalSampler")
	void UpdateLocalTexture();

	
	/** Change sampling radius (will trigger re-sample) */
	UFUNCTION(BlueprintCallable, Category="LocalSampler")
	void SetWorldSampleRadius(float NewRadius);

	//local RenderTarget setter and getter
	UFUNCTION(BlueprintCallable, Category="LocalSampler")
	void SetLocalRenderTarget(UTextureRenderTarget2D* InRT);
	
	UFUNCTION(BlueprintCallable, Category="LocalSampler")
	UTextureRenderTarget2D* GetLocalRenderTarget() const { return LocalMaskRT; }

private:
	bool PrepareSetups();

	// Helper for loading the material and return mid from it
	bool CreateProjectionMID();
	
	bool ShouldRunClientLogic() const;// this is for preventing server to run the update. only cliet or stand alone does
	
	//Internal helpers
	void RebuildLocalBounds(const FVector& WorldCenter);
	void UpdateOverlappingTiles();
	void DrawTilesIntoLocalRT();

private:
	void DebugDrawRT();// this will copy the rt of LocalMaskRT and draw the same thing to the debug rt
	
protected:
	UPROPERTY(EditAnywhere, Category="LocalSampler|Debug")
	bool bDrawDebugRT = false;

	UPROPERTY(EditAnywhere, Category="LocalSampler|Debug")
	TObjectPtr<UTextureRenderTarget2D> DebugRT;// check what is being drawn in the content browser
	
	/** Local merged obstacle/height mask */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LocalSampler|Render")
	TObjectPtr<UTextureRenderTarget2D> LocalMaskRT;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> ScratchRT;// this will be used for storing prev rt. ping-pong bitches

	UPROPERTY(EditDefaultsOnly, Category="LocalSampler|Render")
	TObjectPtr<UMaterialInterface> ProjectionMaterial;
	
	// MID made from the upper material
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ProjectionMID;

	//MID Param names
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LocalSampler|Render")
	FName MIDParam_TileTexture = TEXT("MergingTile");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LocalSampler|Render")
	FName MIDParam_PrevTexture = TEXT("PrevTexture");// to prevent flickering, the prev rt is now stored in temp texture
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LocalSampler|Render")
	FName MIDParam_TileCenter = TEXT("TileCenter");
    
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LocalSampler|Render")
	FName MIDParam_TileSize = TEXT("TileSize");
    
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LocalSampler|Render")
	FName MIDParam_TileRotation = TEXT("TileRotation");

	
	// no longer needed. can merge rt without material
	
	// Sampling Settings

	/* World-space radius of the local sampling area */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LocalSampler|Settings")
	float WorldSampleRadius = 512.f;
	
	/** Distance threshold before re-sampling */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LocalSampler|Settings")
	float UpdateDistanceThreshold = 100.f;
	
	// Cached State
	/** Last world-space center used for sampling */
	UPROPERTY(Transient)
	FVector LastSampleCenter = FVector::ZeroVector;
	/** Cached local world bounds */
	UPROPERTY(Transient)
	FBox2D LocalWorldBounds;
	/** Indices for overlapping Textures on local RT */
	UPROPERTY(Transient)
	TArray<int32> ActiveTileIndices;

private:


	//Subsystem

	UPROPERTY(Transient)
	TObjectPtr<UVisionSubsystem> ObstacleSubsystem;

};
