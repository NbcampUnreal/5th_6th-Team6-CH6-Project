#pragma once

#include "CoreMinimal.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "RTBrushTypes.generated.h"

// Per-frame data submitted by each UFoliageRTInvokerComponent to URTDrawManager.
// The invoker computes this data and owns the MIDs.
// The draw manager renders using the MIDs — never touches parameters.
USTRUCT()
struct MATERIALDRIVENINTERACTION_API FRTInvokerFrameData
{
	GENERATED_BODY()

	// ── RT draw coordinates ───────────────────────────────────────────────────

	// Brush centre in RT UV space [0,1] for this cell
	FVector2D CellUV = FVector2D::ZeroVector;

	// Brush half-extents in UV space
	FVector2D UVExtent = FVector2D::ZeroVector;

	// ── Locomotion encoding ───────────────────────────────────────────────────

	// Velocity encoded [0,1] — 0.5 = stationary
	FVector2D EncodedVelocity = FVector2D(0.5f, 0.5f);

	// Owner yaw in degrees — brush material uses this to orient push direction
	float OwnerYaw = 0.f;

	// Raw GameTime at this frame — stamped into ImpulseRT.G
	float GameTime = 0.f;

	// ── Pool slot ─────────────────────────────────────────────────────────────

	// Pool slot this invoker is currently occupying
	int32 SlotIndex = INDEX_NONE;

	// Cell this invoker is currently in
	FIntPoint CellIndex = FIntPoint(INT32_MIN, INT32_MIN);

	// True if invoker moved to a new texel this frame — gates impulse stamp
	bool bShouldStamp = false;

	// ── Brush materials ───────────────────────────────────────────────────────

	// Created once in BeginPlay by the invoker, reused every frame.
	// Parameters are set by the invoker before submission.
	// Continuous — drawn every frame, no trace
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MID_Continuous = nullptr;

	// Impulse — drawn only when bShouldStamp = true, leaves persistent trace
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MID_Impulse = nullptr;
};