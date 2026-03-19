#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RTMPCUpdater.generated.h"

struct FRTPoolEntry;
class URTPoolManager;
class UMaterialParameterCollection;
class UMaterialParameterCollectionInstance;
class UMaterialInstanceDynamic;
class UTextureRenderTarget2D;

// ──────────────────────────────────────────────────────────────────────────────
// URTMPCUpdater
//
// Attach to any persistent actor (GameMode, GameState, a dedicated BP actor).
// Each tick it pushes the active pool state into a UMaterialParameterCollection
// so every foliage material in the world can read cell origins and cell size
// without a direct C++ reference.
//
// Because UMaterialParameterCollection does not support texture parameters,
// RT texture handles are distributed separately:
//   • Register foliage MIDs via RegisterFoliageMID().
//   • On OnCellAssigned / OnCellReclaimed, textures are pushed to all
//     registered MIDs reactively — not every frame.
//
// MPC parameter layout (declare these in your MPC asset):
//
//   Scalars
//     "FRT_CellSize"         — world-space cell side length (Unreal units)
//     "FRT_ActiveSlotCount"  — number of currently occupied slots
//
//   Vectors  (one per slot, up to FRT_MAX_POOL_SLOTS = 16)
//     "FRT_Slot0_Data" … "FRT_Slot15_Data"
//       .X = CellOriginWS.X
//       .Y = CellOriginWS.Y
//       .Z = 1.0 if occupied, 0.0 if free
//       .W = slot index (debug / material-side switch)
//
// Per-MID texture parameter layout (set on each registered foliage MID):
//     "FRT_Slot0_ImpulseRT"    … "FRT_Slot15_ImpulseRT"
//     "FRT_Slot0_ContinuousRT" … "FRT_Slot15_ContinuousRT"
//
// Foliage material recipe (pseudo-code):
//   for each slot N:
//     SlotData   = MPC.FRT_SlotN_Data
//     CellOrigin = SlotData.XY
//     IsActive   = SlotData.Z > 0.5
//     UV         = (WorldPos.XY - CellOrigin) / MPC.FRT_CellSize
//     InCell     = IsActive && all(UV >= 0) && all(UV <= 1)
//   Pick the slot where InCell == true, sample its RT pair.
// ──────────────────────────────────────────────────────────────────────────────

// Maximum number of slots for which MPC parameters are declared.
// Must match the number of "FRT_SlotN_Data" / "FRT_SlotN_*RT" entries in your
// MPC asset.  Never exceeds URTPoolSettings::PoolSize (clamped to 16).
static constexpr int32 FRT_MAX_POOL_SLOTS = 16;

UCLASS(ClassGroup = "Foliage RT", meta = (BlueprintSpawnableComponent))
class MATERIALDRIVENINTERACTION_API URTMPCUpdater : public UActorComponent
{
	GENERATED_BODY()

public:

	URTMPCUpdater();

	// ── UActorComponent ───────────────────────────────────────────────────────

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	// ── Configuration ─────────────────────────────────────────────────────────

	/**
	 * The MPC asset that receives cell-origin vectors and cell-size each tick.
	 * Create this asset in the editor (right-click → Materials → Material
	 * Parameter Collection) and declare all FRT_* parameters listed above.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|MPC")
	TObjectPtr<UMaterialParameterCollection> ParameterCollection;

	// ── Foliage MID registration ──────────────────────────────────────────────

	/**
	 * Register a foliage MID to receive RT texture parameter updates.
	 * Call this once per foliage component on BeginPlay (or on spawn).
	 * The MID will be updated immediately with the current slot state and then
	 * reactively whenever a slot is assigned or reclaimed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Foliage RT|MPC")
	void RegisterFoliageMID(UMaterialInstanceDynamic* MID);

	/**
	 * Unregister a foliage MID (e.g. when the foliage component is destroyed).
	 */
	UFUNCTION(BlueprintCallable, Category = "Foliage RT|MPC")
	void UnregisterFoliageMID(UMaterialInstanceDynamic* MID);

	/**
	 * Immediately push all current slot RT textures to a single MID.
	 * Useful when a foliage component spawns mid-game after slots are active.
	 */
	UFUNCTION(BlueprintCallable, Category = "Foliage RT|MPC")
	void ForceRefreshMID(UMaterialInstanceDynamic* MID);

	// ── Utility ───────────────────────────────────────────────────────────────

	/**
	 * Returns the MPC instance for this world — use in Blueprint to read
	 * FRT_* parameters directly (e.g. for custom foliage logic).
	 */
	UFUNCTION(BlueprintPure, Category = "Foliage RT|MPC")
	UMaterialParameterCollectionInstance* GetMPCInstance() const;

private:

	// ── Internal ──────────────────────────────────────────────────────────────

	/** Push cell-origin vectors and cell-size scalar to the MPC. */
	void PushVectorDataToMPC();

	/** Push RT texture references for slot SlotIndex to all registered MIDs. */
	void PushTexturesForSlot(int32 SlotIndex);

	/** Push RT texture references for ALL slots to a single MID. */
	void PushAllTexturesToMID(UMaterialInstanceDynamic* MID);

	/** Build the cached parameter FNames for slot N (computed once at init). */
	void BuildParameterNames();

	// ── Pool manager delegate handles — bound in BeginPlay, unbound in EndPlay ─
	UFUNCTION()
	void OnCellAssigned(FIntPoint CellIndex, int32 SlotIndex);

	UFUNCTION()
	void OnCellReclaimed(FIntPoint CellIndex, int32 SlotIndex);

	// ── Cached references ─────────────────────────────────────────────────────

	UPROPERTY()
	TObjectPtr<URTPoolManager> PoolManager;

	/** All foliage MIDs currently receiving texture updates. */
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> RegisteredMIDs;

	// ── Pre-built parameter name arrays (indexed by slot) ─────────────────────
	// Built once in BeginPlay — avoids FName construction on hot path.

	TArray<FName> ParamName_SlotData;        // "FRT_Slot0_Data" … N
	TArray<FName> ParamName_ImpulseRT;       // "FRT_Slot0_ImpulseRT" … N
	TArray<FName> ParamName_ContinuousRT;    // "FRT_Slot0_ContinuousRT" … N

	// MPC scalar parameter names (constant)
	static const FName PN_CellSize;
	static const FName PN_ActiveSlotCount;

	// Debug — mirrors live pool so Details panel shows RT thumbnails during PIE
	UPROPERTY(VisibleAnywhere, Category = "Foliage RT|Debug")
	TArray<FRTPoolEntry> DebugPool;
};
