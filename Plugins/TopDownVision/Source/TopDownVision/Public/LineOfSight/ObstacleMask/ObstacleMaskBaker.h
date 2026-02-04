// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LineOfSight/VisionData.h"// enum for obstacle types
#include "LineOfSight/WorldObstacle/ObstacleData.h"// Obstacle data, enum for command
#include "LineOfSight/WorldObstacle/LevelObstacleTileData.h"// the linker data asset for editor world and runtime world
#include "ObstacleMaskBaker.generated.h"


// Forward declares
class UBoxComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;


UCLASS()
class TOPDOWNVISION_API AObstacleMaskBaker : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AObstacleMaskBaker();

protected:
	virtual void BeginPlay() override;

#if WITH_EDITOR
	
public:
	//Editor command 
	UFUNCTION(CallInEditor, Category="ObstacleMask|Editor")
	void BakeObstacleMask();
	
	void RegisterTile();//inside the Bake function
	
	UFUNCTION(CallInEditor, Category="ObstacleMask|Editor")
	void ClearLocalData();

	//Editor World Iteration Command for replacing multi delegate method

	//wrapper command
	void CommandToAllBakers(EObstacleBakeRequest Request);
	
	UFUNCTION(CallInEditor, Category="Obstacle Mask")
	void BakeTextureToAllBakers() { CommandToAllBakers(EObstacleBakeRequest::Bake); }
	UFUNCTION(CallInEditor, Category="Obstacle Mask")
	void RebuildTextureToAllBakers() { CommandToAllBakers(EObstacleBakeRequest::Rebuild); }
	UFUNCTION(CallInEditor, Category="Obstacle Mask")
	void ClearTextureToAllBakers() { CommandToAllBakers(EObstacleBakeRequest::Clear); }

private:
	// internal helper

	bool GetObstacleActors( // for getting the actors to only render them
		EObstacleType ObstacleType,
		//out
		TArray<AActor*>& OutActors) const;
	
	UTextureRenderTarget2D* CaptureObstaclePass(const TArray<AActor*>& ShowOnlyActors);

	static FMatrix BuildWorldToUV(const FBox2D& WorldBounds);

	void OnBakeRequested(EObstacleBakeRequest Request);// delegate trigger when command is given
	//!!!! --> wont work in the editor. but keep it just in case !!!

	UTextureRenderTarget2D* CaptureSceneDepthRT(const TArray<AActor*>& ObstacleActors);

	UTextureRenderTarget2D* CaptureObstacleDepthRT(const TArray<AActor*>& ObstacleActors);

	UTextureRenderTarget2D* MakeBinaryMaskRT(
		UTextureRenderTarget2D* SceneDepthRT,
		UTextureRenderTarget2D* ObstacleDepthRT);

	void MergeRTs_WithMaterial(
		UTextureRenderTarget2D* RT_Shadow,
		UTextureRenderTarget2D* RT_Low,
		UTextureRenderTarget2D* OutRT);
	
#endif

public:
	//Getter function for getting texture
	UFUNCTION(BlueprintCallable, Category="ObstacleMask")
	UTexture2D* GetBakedObstacleMask() const;

	//Debug for checking if the mask was made correctly
	UFUNCTION(BlueprintCallable, Category="ObstacleMask")
	UMaterialInstanceDynamic* GetDebugMID() const {return DebugMaskMID;}
	
protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldObstacles")
	FRotator SceneCaptureWorldRotation=FRotator(-90.f, 90.f, 0.f);

	
	/** Size of the tile in world units (orthographic width/height) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ObstacleMask|Settings")
	float TileSize = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldObstacles")
	int32 MinResolution = 64;
	/** Max pixel resolution for the capture (clamp) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldObstacles")
	int32 MaxResolution = 1024;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="WorldObstacles")
	int32 PixelResolution;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldObstacles")
	float WorldUnitToPixelRatio=1.f;

	//For making binary Mask texture
	UPROPERTY(EditAnywhere, Category="ObstacleMask|Materials")
	UMaterialInterface* BinaryMaskMaterial;
	//params
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldObstacles")
	FName MIDParam_SceneDepthTex=NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldObstacles")
	FName MIDParam_ObstacleDepthTex=NAME_None;

	//For merging the texture
	UPROPERTY(EditAnywhere, Category="ObstacleMask|Materials")
	UMaterialInterface* MergeMaskMaterial;
	//params
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldObstacles")
	FName MIDParam_RMask=NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldObstacles")
	FName MIDParam_GMask=NAME_None;

	//Debug for checking if the R and G binary mask texture is captured correclty
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WorldObstacles|Debug")
	UMaterialInstanceDynamic* DebugMaskMID = nullptr;
	
	/** Volumes that define the obstacle area */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ObstacleMask|Settings")
	UBoxComponent* BoxVolume;

	/** Root path for generated textures in Content Browser */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ObstacleMask|Settings")
	FString AssetSavePath;

	/** Scene capture component used for baking */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ObstacleMask|Components")
	USceneCaptureComponent2D* SceneCaptureComp;

	//Cached Texture
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ObstacleMask")
	TObjectPtr<UTexture2D> LastBakedTexture;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ObstacleMask|EditorWorldLink")
	TObjectPtr<ULevelObstacleData> TileDataAsset;

	// Obstacle Type Tag
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ObstacleMask")
	FName ObjectType_ShadowCastable=TEXT("LOS_Shadow");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ObstacleMask")
	FName ObjectType_LowObject=TEXT("LOS_Low");
};
