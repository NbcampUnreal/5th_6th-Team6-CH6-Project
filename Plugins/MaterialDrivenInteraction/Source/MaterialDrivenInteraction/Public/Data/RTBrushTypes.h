#pragma once

#include "CoreMinimal.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "RTBrushTypes.generated.h"

USTRUCT()
struct MATERIALDRIVENINTERACTION_API FRTInvokerFrameData
{
	GENERATED_BODY()

	// ── RT draw coordinates ───────────────────────────────────────────────────

	FVector2D CellUV    = FVector2D::ZeroVector;
	FVector2D UVExtent  = FVector2D::ZeroVector;

	// ── Locomotion encoding ───────────────────────────────────────────────────

	// Velocity encoded [0,1] — 0.5 = stationary
	FVector2D EncodedVelocity = FVector2D(0.5f, 0.5f);

	float OwnerYaw = 0.f;

	// frac(GameTime / Period) — stamped into ImpulseRT.A every tick via MAX
	float GameTimeFrac = 0.f;

	// ── Pool slot ─────────────────────────────────────────────────────────────

	int32    SlotIndex = INDEX_NONE;
	FIntPoint CellIndex = FIntPoint(INT32_MIN, INT32_MIN);

	// bShouldStamp removed — impulse drawn every tick, MAX handles preservation

	// ── Brush materials ───────────────────────────────────────────────────────

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MID_Continuous = nullptr;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MID_Impulse = nullptr;
};