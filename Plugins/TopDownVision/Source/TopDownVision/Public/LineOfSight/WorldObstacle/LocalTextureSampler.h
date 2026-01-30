// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "LineOfSight/WorldObstacle/WorldObstacleData.h" //Tile
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
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;
public:

	/** Force update of the local texture (teleport, vision change, etc.) */
	UFUNCTION(BlueprintCallable, Category="LocalSampler")
	void UpdateLocalTexture();

	

	/** Change sampling radius (will trigger re-sample) */
	UFUNCTION(BlueprintCallable, Category="LocalSampler")
	void SetWorldSampleRadius(float NewRadius);

	/** Get local RT for materials (LOS, fog, foliage, etc.) */
	UFUNCTION(BlueprintCallable, Category="LocalSampler")
	UTextureRenderTarget2D* GetLocalRenderTarget() const { return LocalMaskRT; }

	UFUNCTION(BlueprintCallable, Category="LocalSampler")
	UMaterialInstanceDynamic* GetMID() const {return ProjectionMID;}

private:
	void PrepareSetups();
	
protected:
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LocalSampler|Render")
	bool bUseCPU=true;

	/** Local merged obstacle/height mask */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LocalSampler|Render")
	TObjectPtr<UTextureRenderTarget2D> LocalMaskRT;

	/** Material used to project baked tiles into the local RT */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LocalSampler|Render")
	TObjectPtr<UMaterialInterface> TileProjectionMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ProjectionMID;

	//MID Param names
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LocalSampler|Render")
	FName MIDParam_TextureObj =NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LocalSampler|Render")
	FName MIDParam_TileWorldMin =NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LocalSampler|Render")
	FName MIDParam_TileWorldMax =NAME_None;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LocalSampler|Render")
	FName MIDParam_LocalWorldMin =NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LocalSampler|Render")
	FName MIDParam_LocalWorldMax =NAME_None;

	
	// Sampling Settings

	/** World-space radius of the local sampling area */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LocalSampler|Settings")
	float WorldSampleRadius = 512.f;

	/** Resolution of the local render target */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LocalSampler|Settings")
	int32 LocalResolution = 256;

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
	//Internal helpers
	void RebuildLocalBounds(const FVector& WorldCenter);
	void UpdateOverlappingTiles();
	void DrawTilesIntoLocalRT();

	//Subsystem

	UPROPERTY(Transient)
	TObjectPtr<UVisionSubsystem> ObstacleSubsystem;

};
