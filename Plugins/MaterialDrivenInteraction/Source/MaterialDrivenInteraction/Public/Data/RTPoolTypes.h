#pragma once

#include "CoreMinimal.h"
#include "Engine/TextureRenderTarget2D.h"
#include "RTPoolTypes.generated.h"

// ──────────────────────────────────────────────────────────────────────────────
// FRTPoolEntry
//
// One slot in the pool.  Both RTs are pre-allocated at subsystem init and
// never destroyed — only reassigned between cells.
//
// Two-RT design:
//   ImpulseRT    RTF_RGBA32f — stamp-and-snap.  Written only when the brush
//                enters a new pixel.  R=vel X at stamp, G=GameTime (float —
//                needs 32f!), B=vel Y at stamp, A=height mask.
//                Kept alive through the full decay window so the foliage
//                material can read TimeDelta = CurrentTime - G.
//
//   ContinuousRT RTF_RGBA8   — painted every frame while the invoker is
//                present.  R=live vel X (0.5=zero), B=live vel Y, A=weight.
//                Cleared to black the moment the last invoker leaves —
//                no decay data lives here, so no lazy-clear is needed.
// ──────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct MATERIALDRIVENINTERACTION_API FRTPoolEntry
{
	GENERATED_BODY()

	// Stamp-and-snap RT — float precision, survives the full decay window.
	UPROPERTY(BlueprintReadOnly, Category = "Foliage RT")
	TObjectPtr<UTextureRenderTarget2D> ImpulseRT = nullptr;

	// Live-brush RT — 8-bit is fine, cleared immediately when invokers leave.
	UPROPERTY(BlueprintReadOnly, Category = "Foliage RT")
	TObjectPtr<UTextureRenderTarget2D> ContinuousRT = nullptr;

	// Which grid cell this slot is currently serving.
	// FIntPoint(INT32_MIN, INT32_MIN) == free / unassigned.
	// INT32_MIN is used instead of (-1,-1) because (-1,-1) is a valid cell
	// index for world positions in the negative quadrant (e.g. X=-500 with
	// CellSize=2000 maps to cell -1). Using INT32_MIN guarantees no real
	// world cell can ever collide with the free sentinel.
	UPROPERTY(BlueprintReadOnly, Category = "Foliage RT")
	FIntPoint AssignedCell = FIntPoint(INT32_MIN, INT32_MIN);

	// World-space origin of the assigned cell (bottom-left corner, Z ignored).
	// Foliage materials use this to compute their 0–1 UV.
	UPROPERTY(BlueprintReadOnly, Category = "Foliage RT")
	FVector2D CellOriginWS = FVector2D(-9999999.f, -9999999.f);

	// GetWorld()->GetTimeSeconds() when the last invoker left this cell.
	// -1 means an invoker is still present.
	UPROPERTY(BlueprintReadOnly, Category = "Foliage RT")
	float LastReleaseTime = -1.f;

	// Number of invokers currently registered in this cell.
	// Slot is only eligible for reclaim when this reaches 0 AND decay has elapsed.
	UPROPERTY(BlueprintReadOnly, Category = "Foliage RT")
	int32 ActiveInvokerCount = 0;

	// ImpulseRT needs a lazy clear on next assignment (set at reclaim time).
	// ContinuousRT is cleared eagerly when invokers reach 0 and never needs
	// this flag — it is always black when AssignedCell == (INT32_MIN, INT32_MIN).
	bool bImpulseNeedsClear = false;

	bool IsFree()     const { return AssignedCell == FIntPoint(INT32_MIN, INT32_MIN); }
	bool IsOccupied() const { return !IsFree(); }
};

// ──────────────────────────────────────────────────────────────────────────────
// URTPoolSettings  (developer-facing config, set per project)
// ──────────────────────────────────────────────────────────────────────────────
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Foliage RT Pool"))
class MATERIALDRIVENINTERACTION_API URTPoolSettings : public UObject
{
	GENERATED_BODY()

public:
	URTPoolSettings()
	{
		PoolSize      = 9;
		CellSize      = 2000.f;
		RTResolution  = 512;
		DecayDuration = 10.f;
	}

	// How many render targets to pre-allocate (4 = 2×2 neighbourhood, 9 = 3×3)
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Pool",
		meta = (ClampMin = 1, ClampMax = 16))
	int32 PoolSize;

	// Side length of one world-space grid cell in Unreal units
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Grid",
		meta = (ClampMin = 100.f))
	float CellSize;

	// Pixel resolution of each RT (power of two recommended)
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Pool",
		meta = (ClampMin = 64, ClampMax = 2048))
	int32 RTResolution;

	// Seconds after the last invoker leaves before the slot may be reclaimed.
	// Must be >= the longest foliage decay you author in the material.
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Pool",
		meta = (ClampMin = 0.f))
	float DecayDuration;
};
