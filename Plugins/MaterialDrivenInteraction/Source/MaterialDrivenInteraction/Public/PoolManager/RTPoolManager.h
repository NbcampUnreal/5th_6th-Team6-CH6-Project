#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/RTPoolTypes.h"
#include "RTPoolManager.generated.h"

class UTextureRenderTarget2D;

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

	// Called by each invoker component every tick.
	// Frame-guarded internally — only executes once per frame regardless of invoker count.
	void Tick(float DeltaTime);

	// ── Invoker API ───────────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "Foliage RT|Pool")
	int32 RegisterInvoker(FVector WorldLocation);

	UFUNCTION(BlueprintCallable, Category = "Foliage RT|Pool")
	void UnregisterInvoker(FVector WorldLocation);

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

	int32 FindSlotForCell(FIntPoint CellIndex) const;
	int32 AssignFreeSlot(FIntPoint CellIndex);
	int32 EvictOldestReleasedSlot();

	void ClearImpulseSlot(int32 SlotIndex);
	void ClearContinuousSlot(int32 SlotIndex);
	void ReclaimExpiredSlots();

	UPROPERTY()
	TArray<FRTPoolEntry> Pool;

	// Frame guard — ensures Tick body runs exactly once per frame
	// even when multiple invoker components call it.
	uint64 LastTickFrame = 0;
};