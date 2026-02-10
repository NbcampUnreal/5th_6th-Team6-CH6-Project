// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
//#include "Components/SceneComponent.h"
#include "Components/ActorComponent.h" //-> now actor
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
 *
 *
 * s
 */


//Forward declares
class UTextureRenderTarget2D;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UWorldObstacleSubsystem;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API ULocalTextureSampler : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	ULocalTextureSampler();

protected:
	virtual void BeginPlay() override;

	virtual void OnComponentCreated() override;
	
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

	void SetLocationRoot(USceneComponent* NewRoot);
	//Line of sight comp is now a actor comp with no location. make a function to set the location
	
private:
	void PrepareSetups();

	bool ShouldRunClientLogic() const;

protected:
	
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LocalSampler|Render")
	bool bDrawDebugRT=false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LocalSampler|Render")
	TObjectPtr<UTextureRenderTarget2D> DebugRT;
	
	/** Local merged obstacle/height mask */
  	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LocalSampler|Render")
	TObjectPtr<UTextureRenderTarget2D> LocalMaskRT;
	
	
	// Sampling Settings

	/** World-space radius of the local sampling area */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LocalSampler|Settings")
	float WorldSampleRadius = 512.f;

	/** Resolution of the local render target */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LocalSampler|Settings")
	int32 LocalResolution = 256;
	
	
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
	TObjectPtr<UWorldObstacleSubsystem> ObstacleSubsystem;

	UPROPERTY(Transient)
	TWeakObjectPtr<USceneComponent> SourceRoot;// to read the world location
};
