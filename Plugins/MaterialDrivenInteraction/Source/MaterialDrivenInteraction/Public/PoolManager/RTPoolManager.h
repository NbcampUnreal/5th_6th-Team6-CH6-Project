#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/RTPoolTypes.h"
#include "RTPoolManager.generated.h"

class UTextureRenderTarget2D;
class UMaterialParameterCollection;
class UMaterialParameterCollectionInstance;

// ──────────────────────────────────────────────────────────────────────────────
// Broadcast when a cell is assigned or reclaimed — useful for debugging
// and for driving MPC updates in Blueprint.
// ──────────────────────────────────────────────────────────────────────────────
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCellAssigned,
	FIntPoint, CellIndex, int32, PoolSlotIndex);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCellReclaimed,
	FIntPoint, CellIndex, int32, PoolSlotIndex);

// ──────────────────────────────────────────────────────────────────────────────
// URTPoolManager
//
// UWorldSubsystem — one instance per world, auto-created, Blueprint-accessible
// via GetWorld → GetSubsystem<URTPoolManager>.
//
// Responsibilities
//   • Pre-allocate the RT pool at world init.
//   • Convert world positions → cell indices → pool slots.
//   • Track invoker reference counts per cell.
//   • Reclaim and clear slots after DecayDuration has elapsed.
//   • Expose RT + UV data so the foliage material can sample correctly.
// ──────────────────────────────────────────────────────────────────────────────
UCLASS(BlueprintType)
class MATERIALDRIVENINTERACTION_API URTPoolManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	// ── USubsystem ────────────────────────────────────────────────────────────

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	// Tick is driven manually by the invoker each frame (no actor tick overhead)
	void Tick(float DeltaTime);

	// ── Invoker API  (called by the pawn/NPC component) ───────────────────────

	/**
	 * Register an invoker at WorldLocation.
	 * Finds or assigns a pool slot for the enclosing cell.
	 * Returns the slot index, or -1 if the pool is full.
	 */
	UFUNCTION(BlueprintCallable, Category = "Foliage RT|Pool")
	int32 RegisterInvoker(FVector WorldLocation);

	/**
	 * Unregister an invoker.  Starts the decay clock when the last invoker
	 * leaves the cell.
	 */
	UFUNCTION(BlueprintCallable, Category = "Foliage RT|Pool")
	void UnregisterInvoker(FVector WorldLocation);

	// ── Foliage material query API ────────────────────────────────────────────

	/**
	 * Given a foliage instance's world XY, returns both render targets and the
	 * normalised UV (0-1) within them.
	 * Returns false when no slot covers this position (foliage should skip WPO).
	 *
	 * OutImpulseRT    — sample for TimeDelta decay and wobble (R=velX, G=time, B=velY, A=mask)
	 * OutContinuousRT — sample for live static lean    (R=velX, B=velY, A=weight)
	 */
	UFUNCTION(BlueprintCallable, Category = "Foliage RT|Query")
	bool GetRTForWorldPosition(FVector WorldLocation,
	                           UTextureRenderTarget2D*& OutImpulseRT,
	                           UTextureRenderTarget2D*& OutContinuousRT,
	                           FVector2D&               OutUV) const;

	/**
	 * Returns both RTs and the cell-origin for a slot index.
	 * Used to push data into the MPC each frame.
	 */
	UFUNCTION(BlueprintCallable, Category = "Foliage RT|Query")
	bool GetSlotData(int32 SlotIndex,
	                 UTextureRenderTarget2D*& OutImpulseRT,
	                 UTextureRenderTarget2D*& OutContinuousRT,
	                 FVector2D&               OutCellOriginWS) const;

	// ── Utility ───────────────────────────────────────────────────────────────

	/** Converts a world XY position to the integer cell index. */
	UFUNCTION(BlueprintPure, Category = "Foliage RT|Utility")
	FIntPoint WorldToCell(FVector WorldLocation) const;

	/** Returns the world-space bottom-left origin of a cell. */
	UFUNCTION(BlueprintPure, Category = "Foliage RT|Utility")
	FVector2D CellToWorldOrigin(FIntPoint CellIndex) const;

	/** Returns the number of active (occupied) pool slots. */
	UFUNCTION(BlueprintPure, Category = "Foliage RT|Debug")
	int32 GetActiveSlotCount() const;

	/** Read-only view of the pool — useful for debug widgets. */
	UFUNCTION(BlueprintPure, Category = "Foliage RT|Debug")
	const TArray<FRTPoolEntry>& GetPool() const { return Pool; }

	// ── Events ────────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "Foliage RT|Events")
	FOnCellAssigned OnCellAssigned;

	UPROPERTY(BlueprintAssignable, Category = "Foliage RT|Events")
	FOnCellReclaimed OnCellReclaimed;

	// ── Config (mirrors URTPoolSettings, cached at init) ─────────────────────

	UPROPERTY(BlueprintReadOnly, Category = "Foliage RT|Config")
	int32 PoolSize = 9;

	UPROPERTY(BlueprintReadOnly, Category = "Foliage RT|Config")
	float CellSize = 2000.f;

	UPROPERTY(BlueprintReadOnly, Category = "Foliage RT|Config")
	int32 RTResolution = 512;

	UPROPERTY(BlueprintReadOnly, Category = "Foliage RT|Config")
	float DecayDuration = 10.f;

private:

	// ── Internal helpers ──────────────────────────────────────────────────────

	/** Creates a pre-cleared RGBA32f RT for stamp-and-snap impulse data. */
	UTextureRenderTarget2D* CreateImpulseRT(const FName& Name) const;

	/** Creates a pre-cleared RGBA8 RT for continuous live-brush painting. */
	UTextureRenderTarget2D* CreateContinuousRT(const FName& Name) const;

	/** Returns pool slot index for a cell, or INDEX_NONE if not assigned. */
	int32 FindSlotForCell(FIntPoint CellIndex) const;

	/** Assigns a free slot to CellIndex. Lazily clears ImpulseRT if flagged. */
	int32 AssignFreeSlot(FIntPoint CellIndex);

	/**
	 * When the pool is exhausted, forcibly reclaims the slot that was released
	 * longest ago (lowest active invoker risk). Returns the freed slot index,
	 * or INDEX_NONE if all slots still have active invokers.
	 */
	int32 EvictOldestReleasedSlot();

	/** Clears ImpulseRT to (0,0,0,0) — called lazily on next assignment. */
	void ClearImpulseSlot(int32 SlotIndex);

	/** Clears ContinuousRT to (0,0,0,0) — called eagerly when invokers reach 0. */
	void ClearContinuousSlot(int32 SlotIndex);

	/** Reclaim all slots whose invoker count is 0 and decay time has passed. */
	void ReclaimExpiredSlots();

	// ── State ─────────────────────────────────────────────────────────────────

	UPROPERTY()
	TArray<FRTPoolEntry> Pool;
};
