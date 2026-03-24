#pragma once

#include "CoreMinimal.h"
#include "Engine/TextureRenderTarget2D.h"
#include "RTPoolTypes.generated.h"

USTRUCT(BlueprintType)
struct MATERIALDRIVENINTERACTION_API FRTPoolEntry
{
	GENERATED_BODY()

	// ImpulseRT — RGBA32f, stamp-and-snap.
	// Written once per new pixel entry by the invoker brush material.
	//   R = velocity X encoded [0,1]  (0.5 = stationary)
	//   G = ImpactTime raw float      (32f required — TimeDelta = Now - G)
	//   B = velocity Y encoded [0,1]
	//   A = weight * mask             (0 = texel never stamped, gates FRT_DampedSine)
	// Clear color: (0,0,0,0) — A=0 ensures DampedSine guard fires, no phantom wobble.
	// Cleared lazily on slot reassignment via bImpulseNeedsClear.
	UPROPERTY(BlueprintReadOnly, Category = "Foliage RT")
	TObjectPtr<UTextureRenderTarget2D> ImpulseRT = nullptr;

	// ContinuousRT — RGBA8, repainted every frame while invoker is present.
	// Written by the invoker's brush material which computes push direction per-texel.
	// No trace — cleared to neutral at the start of every frame by RTPoolManager::Tick.
	//   R = push direction X normalized, encoded [0,1]  (0.5 = no direction)
	//   G = push direction Y normalized, encoded [0,1]  (0.5 = no direction)
	//   B = height mask (reserved for future use — always 0)
	//   A = weight * mask  (0 = no brush present, gates static lean)
	// Clear color: (0.5, 0.5, 0, 0) — neutral direction, no weight.
	// Also cleared eagerly when the last invoker leaves the cell.
	UPROPERTY(BlueprintReadOnly, Category = "Foliage RT")
	TObjectPtr<UTextureRenderTarget2D> ContinuousRT = nullptr;

	// FIntPoint(INT32_MIN, INT32_MIN) == free / unassigned.
	// INT32_MIN used instead of (-1,-1) because (-1,-1) is a valid cell
	// for world positions in the negative quadrant.
	UPROPERTY(BlueprintReadOnly, Category = "Foliage RT")
	FIntPoint AssignedCell = FIntPoint(INT32_MIN, INT32_MIN);

	// World-space bottom-left origin of the assigned cell.
	UPROPERTY(BlueprintReadOnly, Category = "Foliage RT")
	FVector2D CellOriginWS = FVector2D(-9999999.f, -9999999.f);

	// -1 = invoker still present. >= 0 = decay clock running.
	UPROPERTY(BlueprintReadOnly, Category = "Foliage RT")
	float LastReleaseTime = -1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Foliage RT")
	int32 ActiveInvokerCount = 0;

	// ImpulseRT lazy-clear flag — set at reclaim, cleared on next assignment.
	bool bImpulseNeedsClear = false;

	bool IsFree()     const { return AssignedCell == FIntPoint(INT32_MIN, INT32_MIN); }
	bool IsOccupied() const { return !IsFree(); }
};

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

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Pool",
		meta = (ClampMin = 1, ClampMax = 16))
	int32 PoolSize;

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Grid",
		meta = (ClampMin = 100.f))
	float CellSize;

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Pool",
		meta = (ClampMin = 64, ClampMax = 2048))
	int32 RTResolution;

	// Must be >= the longest foliage decay authored in the material.
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Pool",
		meta = (ClampMin = 0.f))
	float DecayDuration;

	// Pre-created RT assets — assign RT_FoliageImpulse_00 … _15 here.
	// These same assets must be set as default values for FRT_SlotN_ImpulseRT
	// in M_Foliage_Consumer so the material always references the correct RT.
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Pool|RenderTargets")
	TArray<TSoftObjectPtr<UTextureRenderTarget2D>> ImpulseRTs;

	// Pre-created RT assets — assign RT_FoliageContinuous_00 … _15 here.
	// Same requirement as above for FRT_SlotN_ContinuousRT.
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Pool|RenderTargets")
	TArray<TSoftObjectPtr<UTextureRenderTarget2D>> ContinuousRTs;
};