// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"



// FD
class UCameraComponent;
class APlayerCameraManager;

// Shared data struct

enum class ECameraProjectionType : uint8 // when ortho-> just same radius.
{
	Perspective,
	Orthographic
};

struct FCameraFrustumParams//requirments as struct
{
	float                  VerticalFOVDeg       = 90.f;
	float                  AspectRatio          = 1.777f;
	float                  FrustumHalfAngleRad  = 0.f;
	FVector                CameraLocation       = FVector::ZeroVector;
	FVector                CameraForward        = FVector::ForwardVector;
	ECameraProjectionType  ProjectionType       = ECameraProjectionType::Perspective;
};

// Helper class

class TOPDOWNVISION_API FFrustumProjectionMatcherHelper
{
public:

	//  Extraction 

	static bool ExtractFromCameraComponent(
		const UCameraComponent*  CameraComponent,
		FCameraFrustumParams&    OutParams
	);

	static bool ExtractFromCameraManager(
		const APlayerCameraManager*  CameraManager,
		FCameraFrustumParams&        OutParams
	);

	// Computation 

	static float CalculateFrustumHalfAngleRad(
		const FCameraFrustumParams& Params
	);

	static float CalculateSphereRadiusAtDepth(
		const FCameraFrustumParams& Params,
		float TargetDistance,
		float TargetVisibleRadius,
		float SphereDepth
	);
};