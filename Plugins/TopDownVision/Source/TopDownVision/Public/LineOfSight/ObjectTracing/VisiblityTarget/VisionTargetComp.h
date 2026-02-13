// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VisionTargetComp.generated.h"

/**
 *  To replace the current approach of evaluating sample point overlapping with volumes,
 *   --> this takes the RT made by LOS comp and reads the points of 2d sample points.
 *
 *  
 */

//ForwardDeclares
class UMeshComponent;
class UMaterialInstanceDynamic;
class UTextureRenderTarget2D;

UCLASS()
class TOPDOWNVISION_API UVisionTargetComp : public UActorComponent
{
	GENERATED_BODY()
public:

	UVisionTargetComp();

	/*
	/** check if the point are overlapping with the obstacle entirely or not #1#
	bool EvaluateVisibility(
		UTextureRenderTarget2D* VisionRenderTarget
		);

	/** Force visibility state #1#
	void SetVisible(bool bNewVisible);

	bool IsVisible() const { return bIsVisible; }

protected:

	virtual void BeginPlay() override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	/** Build sample points relative to actor #1#
	void InitializeSamplePoints();

	/** Reads render target at world location #1#
	bool IsWorldLocationVisible(
		const FVector& WorldLocation,
		UTextureRenderTarget2D* VisionRenderTarget) const;

	/** Updates material reveal value #1#
	void UpdateReveal(float DeltaTime);

protected:

	/** Mesh to control visibility on  #1#
	UPROPERTY(EditAnywhere, Category="Vision")
	UMeshComponent* TargetMesh;

	/** Parameter name inside material #1#
	UPROPERTY(EditAnywhere, Category="Vision")
	FName RevealParameterName = "Reveal";

	/** Speed of fade in/out #1#
	UPROPERTY(EditAnywhere, Category="Vision")
	float RevealInterpSpeed = 5.f;

	/** 2D sample offsets (relative to actor origin, XY plane) #1#
	UPROPERTY(EditAnywhere, Category="Vision")
	TArray<FVector2D> SamplePoints;

private:

	/** Runtime material instance #1#
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* OwnerMID = nullptr;

	/** Current visibility state #1#
	bool bIsVisible = false;

	/** Smoothed reveal value #1#
	float CurrentRevealValue = 0.f;*/
};