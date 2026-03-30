#pragma once

#include "CoreMinimal.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "RTBrushTypes.generated.h"


// Single interaction RT — RGBA16f, 8-bit packed per channel.
//   R = Pack(PushDirX, VelX)      — direction and velocity X
//   G = Pack(PushDirY, VelY)      — direction and velocity Y
//   B = Pack(HeightMask, Progression) — normal lerp weight + decay value
//   A = unused. alpha is not trustable value. unreal changes it internally and cannot be debugged by rt
// Clear color: Pack(0.5,0.5) for RG neutral direction, 0 for B.


USTRUCT()
struct MATERIALDRIVENINTERACTION_API FRTInvokerFrameData
{
	GENERATED_BODY()

	FVector2D CellUV   = FVector2D::ZeroVector;
	FVector2D UVExtent = FVector2D::ZeroVector;

	// Velocity encoded [0,1] — 0.5 = stationary
	FVector2D EncodedVelocity = FVector2D(0.5f, 0.5f);

	float DecayRate    = 0.95f;
	float BrushWeight  = 1.f;
	float BrushSoftness = 0.5f;

	int32     SlotIndex = INDEX_NONE;
	FIntPoint CellIndex = FIntPoint(INT32_MIN, INT32_MIN);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MID_Interaction = nullptr;
};