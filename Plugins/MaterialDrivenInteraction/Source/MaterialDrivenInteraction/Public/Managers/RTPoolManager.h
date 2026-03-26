#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/RTPoolTypes.h"
#include "RTPoolManager.generated.h"

class UTextureRenderTarget2D;
class UFoliageRTInvokerComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCellAssigned,
	FIntPoint, CellIndex, int32, PoolSlotIndex);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCellReclaimed,
	FIntPoint, CellIndex, int32, PoolSlotIndex);

UCLASS(BlueprintType)
class MATERIALDRIVENINTERACTION_API URTPoolManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	// Frame guard — called by invoker components every frame.
	void Tick(float DeltaTime);

	// ── Priority center ───────────────────────────────────────────────────────

	// Called by URTMPCUpdater every tick.
	// Slots farthest from this point are evicted first.
	void SetPriorityCenter(FVector WorldLocation);
	FVector GetPriorityCenter() const { return PriorityCenter; }

	void SetDrawRadius(float Radius) { DrawRadius = Radius; }
	float GetDrawRadius() const { return DrawRadius; }

	// ── Invoker registration — called once at BeginPlay/EndPlay ───────────────

	// Stores the invoker permanently. Pool evaluation assigns slots internally.
	void RegisterInvoker(UFoliageRTInvokerComponent* Invoker);
	void UnregisterInvoker(UFoliageRTInvokerComponent* Invoker);

	// ── Slot evaluation — called by URTDrawManager each draw cycle ────────────

	// Sorts all registered invokers by distance from PriorityCenter.
	// Assigns the closest N invokers to pool slots (N = PoolSize).
	// Returns the list of invokers that received a slot this cycle.
	// Invokers beyond PoolSize get no slot and are not drawn this cycle.
	TArray<TTuple<UFoliageRTInvokerComponent*, int32>> EvaluateAndAssignSlots();

	// Called by URTDrawManager after each draw cycle.
	void ReclaimExpiredSlots();

	// Called by URTDrawManager after clearing an ImpulseRT.
	void ClearImpulseFlag(int32 SlotIndex);

	// ── Query API ─────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "Foliage RT|Query")
	bool GetRTForWorldPosition(FVector WorldLocation,
	                           UTextureRenderTarget2D*& OutImpulseRT,
	                           UTextureRenderTarget2D*& OutContinuousRT,
	                           FVector2D&               OutUV) const;

	UFUNCTION(BlueprintCallable, Category = "Foliage RT|Query")
	bool GetSlotData(int32 SlotIndex,
	                 UTextureRenderTarget2D*& OutImpulseRT,
	                 UTextureRenderTarget2D*& OutContinuousRT,
	                 FVector2D&               OutCellOriginWS) const;

	// ── Utility ───────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintPure, Category = "Foliage RT|Utility")
	FIntPoint WorldToCell(FVector WorldLocation) const;

	UFUNCTION(BlueprintPure, Category = "Foliage RT|Utility")
	FVector2D CellToWorldOrigin(FIntPoint CellIndex) const;

	UFUNCTION(BlueprintPure, Category = "Foliage RT|Debug")
	int32 GetActiveSlotCount() const;

	UFUNCTION(BlueprintPure, Category = "Foliage RT|Debug")
	const TArray<FRTPoolEntry>& GetPool() const { return Pool; }

	UFUNCTION(BlueprintPure, Category = "Foliage RT|Debug")
	FString GetPoolDebugString() const;

	// ── Events ────────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "Foliage RT|Events")
	FOnCellAssigned OnCellAssigned;

	UPROPERTY(BlueprintAssignable, Category = "Foliage RT|Events")
	FOnCellReclaimed OnCellReclaimed;


	// Bool Check
	bool IsInvokerActive(UFoliageRTInvokerComponent* Invoker) const;

	// ── Config ────────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, Category = "Foliage RT|Config")
	int32 PoolSize = 9;

	UPROPERTY(BlueprintReadOnly, Category = "Foliage RT|Config")
	float CellSize = 2000.f;

	UPROPERTY(BlueprintReadOnly, Category = "Foliage RT|Config")
	int32 RTResolution = 512;

	UPROPERTY(BlueprintReadOnly, Category = "Foliage RT|Config")
	float DecayDuration = 10.f;

private:

	UTextureRenderTarget2D* LoadImpulseRT(int32 SlotIndex) const;
	UTextureRenderTarget2D* LoadContinuousRT(int32 SlotIndex) const;

	// Finds a free pool slot and assigns it.
	// Clears ImpulseRT if bImpulseNeedsClear is set.
	int32 AssignFreeSlot();

	// Evicts the best slot based on distance from PriorityCenter and age.
	// Primary: farthest released slot. Secondary: oldest if equal distance.
	// Last resort: farthest active slot if no released slots exist.
	int32 EvictBestSlot();

	UPROPERTY()
	TArray<FRTPoolEntry> Pool;

	// All registered invokers — permanent until UnregisterInvoker is called.
	UPROPERTY()
	TArray<TWeakObjectPtr<UFoliageRTInvokerComponent>> RegisteredInvokers;

	// Priority center — set by URTMPCUpdater each tick.
	FVector PriorityCenter = FVector::ZeroVector;

	// Frame guard
	uint64 LastTickFrame = 0;

	//filter range
	float DrawRadius = 5000.f;
};