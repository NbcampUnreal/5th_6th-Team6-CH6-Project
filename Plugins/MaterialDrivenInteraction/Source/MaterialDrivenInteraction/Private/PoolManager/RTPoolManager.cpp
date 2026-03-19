// Fill out your copyright notice in the Description page of Project Settings.

#include "PoolManager/RTPoolManager.h"

#include "Data/RTPoolTypes.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/World.h"

// ── USubsystem lifecycle ──────────────────────────────────────────────────────

bool URTPoolManager::ShouldCreateSubsystem(UObject* Outer) const
{
	// Only create in game worlds — skip editor preview, thumbnails, etc.
	UWorld* World = Cast<UWorld>(Outer);
	return World && (World->IsGameWorld() || World->IsPlayInEditor());
}

void URTPoolManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Pull settings from project config (defaults used if config missing)
	if (const URTPoolSettings* Settings = GetDefault<URTPoolSettings>())
	{
		PoolSize      = FMath::Clamp(Settings->PoolSize,     1,    16);
		CellSize      = FMath::Max (Settings->CellSize,      100.f);
		RTResolution  = FMath::Clamp(Settings->RTResolution, 64,  2048);
		DecayDuration = FMath::Max (Settings->DecayDuration, 0.f);
	}

	Pool.Reserve(PoolSize);

	for (int32 i = 0; i < PoolSize; ++i)
	{
		FRTPoolEntry Entry;
		Entry.ImpulseRT    = CreateImpulseRT(
			FName(*FString::Printf(TEXT("FoliageRT_Impulse_%02d"), i)));
		Entry.ContinuousRT = CreateContinuousRT(
			FName(*FString::Printf(TEXT("FoliageRT_Continuous_%02d"), i)));
		Pool.Add(Entry);
	}

	UE_LOG(LogTemp, Log,
		TEXT("[FoliageRT] Pool initialised — %d slots, %.0f-unit cells, %dx%d RT, %.1fs decay"),
		PoolSize, CellSize, RTResolution, RTResolution, DecayDuration);
}

void URTPoolManager::Deinitialize()
{
	// RTs are UPROPERTY-tracked — GC handles destruction.
	// We just nullify our references so nothing re-enters the pool during GC.
	Pool.Empty();

	Super::Deinitialize();
}

// ── Tick (driven by invoker component, not by the actor tick system) ──────────

void URTPoolManager::Tick(float DeltaTime)
{
	ReclaimExpiredSlots();
}

// ── Invoker API ───────────────────────────────────────────────────────────────

int32 URTPoolManager::RegisterInvoker(FVector WorldLocation)
{
	const FIntPoint Cell = WorldToCell(WorldLocation);

	// Already have a slot for this cell?
	int32 SlotIdx = FindSlotForCell(Cell);

	if (SlotIdx == INDEX_NONE)
	{
		SlotIdx = AssignFreeSlot(Cell);
		if (SlotIdx == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[FoliageRT] Pool exhausted — invoker at (%.0f, %.0f) could not get an RT. "
				     "Increase PoolSize or DecayDuration."),
				WorldLocation.X, WorldLocation.Y);
			return INDEX_NONE;
		}
	}

	FRTPoolEntry& Slot = Pool[SlotIdx];

	// Reset the decay clock — an invoker is now active
	Slot.ActiveInvokerCount++;
	Slot.LastReleaseTime = -1.f;   // -1 == "still occupied"

	return SlotIdx;
}

void URTPoolManager::UnregisterInvoker(FVector WorldLocation)
{
	const FIntPoint Cell    = WorldToCell(WorldLocation);
	const int32     SlotIdx = FindSlotForCell(Cell);

	if (SlotIdx == INDEX_NONE)
	{
		// This can legitimately happen if the pool was full when the invoker entered
		return;
	}

	FRTPoolEntry& Slot = Pool[SlotIdx];
	Slot.ActiveInvokerCount = FMath::Max(0, Slot.ActiveInvokerCount - 1);

	if (Slot.ActiveInvokerCount == 0)
	{
		// ContinuousRT holds live velocity — no decay data, so clear it immediately.
		// ImpulseRT is left untouched; the foliage material still reads it for decay.
		ClearContinuousSlot(SlotIdx);

		// Start the decay clock for ImpulseRT reclaim.
		Slot.LastReleaseTime = GetWorld()->GetTimeSeconds();

		UE_LOG(LogTemp, Verbose,
			TEXT("[FoliageRT] Cell (%d,%d) released. ContinuousRT cleared, decay clock started — reclaim in %.1fs."),
			Cell.X, Cell.Y, DecayDuration);
	}
}

// ── Foliage material query API ────────────────────────────────────────────────

bool URTPoolManager::GetRTForWorldPosition(FVector WorldLocation,
                                           UTextureRenderTarget2D*& OutImpulseRT,
                                           UTextureRenderTarget2D*& OutContinuousRT,
                                           FVector2D&               OutUV) const
{
	const FIntPoint Cell    = WorldToCell(WorldLocation);
	const int32     SlotIdx = FindSlotForCell(Cell);

	if (SlotIdx == INDEX_NONE)
	{
		OutImpulseRT    = nullptr;
		OutContinuousRT = nullptr;
		OutUV           = FVector2D::ZeroVector;
		return false;
	}

	const FRTPoolEntry& Slot = Pool[SlotIdx];
	OutImpulseRT    = Slot.ImpulseRT;
	OutContinuousRT = Slot.ContinuousRT;

	// UV = (WorldXY - CellOriginXY) / CellSize, clamped [0,1]
	const FVector2D WorldXY(WorldLocation.X, WorldLocation.Y);
	OutUV = (WorldXY - Slot.CellOriginWS) / CellSize;
	OutUV = OutUV.ClampAxes(0.f, 1.f);

	return true;
}

bool URTPoolManager::GetSlotData(int32 SlotIndex,
                                  UTextureRenderTarget2D*& OutImpulseRT,
                                  UTextureRenderTarget2D*& OutContinuousRT,
                                  FVector2D&               OutCellOriginWS) const
{
	if (!Pool.IsValidIndex(SlotIndex))
	{
		OutImpulseRT    = nullptr;
		OutContinuousRT = nullptr;
		OutCellOriginWS = FVector2D::ZeroVector;
		return false;
	}

	const FRTPoolEntry& Slot = Pool[SlotIndex];
	OutImpulseRT    = Slot.ImpulseRT;
	OutContinuousRT = Slot.ContinuousRT;
	OutCellOriginWS = Slot.CellOriginWS;
	return Slot.IsOccupied();
}

// ── Utility ───────────────────────────────────────────────────────────────────

FIntPoint URTPoolManager::WorldToCell(FVector WorldLocation) const
{
	return FIntPoint(
		FMath::FloorToInt(WorldLocation.X / CellSize),
		FMath::FloorToInt(WorldLocation.Y / CellSize)
	);
}

FVector2D URTPoolManager::CellToWorldOrigin(FIntPoint CellIndex) const
{
	return FVector2D(
		CellIndex.X * CellSize,
		CellIndex.Y * CellSize
	);
}

int32 URTPoolManager::GetActiveSlotCount() const
{
	int32 Count = 0;
	for (const FRTPoolEntry& Slot : Pool)
	{
		if (Slot.IsOccupied()) { ++Count; }
	}
	return Count;
}

// ── Internal helpers ──────────────────────────────────────────────────────────

UTextureRenderTarget2D* URTPoolManager::CreateImpulseRT(const FName& Name) const
{
	UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(
		GetTransientPackage(), Name);

	// RGBA32f required — G stores GameTime as a raw float.
	// RTF_R16f loses precision after ~60s; full RGBA32f keeps all channels
	// at no extra draw-call cost.
	// Layout: R=vel X at stamp, G=GameTime, B=vel Y at stamp, A=height mask
	RT->RenderTargetFormat = RTF_RGBA32f;
	RT->InitAutoFormat(RTResolution, RTResolution);
	RT->ClearColor        = FLinearColor(0.f, 0.f, 0.f, 0.f);
	RT->bAutoGenerateMips = false;
	RT->UpdateResource();

	if (UWorld* World = GetWorld())
	{
		UKismetRenderingLibrary::ClearRenderTarget2D(World, RT, FLinearColor::Black);
	}

	return RT;
}

UTextureRenderTarget2D* URTPoolManager::CreateContinuousRT(const FName& Name) const
{
	UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(
		GetTransientPackage(), Name);

	// RGBA8 is sufficient — values are normalized velocity and weight, no floats.
	// Layout: R=live vel X (0.5=zero), B=live vel Y (0.5=zero), A=weight
	// Black (0,0,0,0) == "no interaction" — must decode cleanly in the material.
	RT->RenderTargetFormat = RTF_RGBA8;
	RT->InitAutoFormat(RTResolution, RTResolution);
	RT->ClearColor        = FLinearColor(0.f, 0.f, 0.f, 0.f);
	RT->bAutoGenerateMips = false;
	RT->UpdateResource();

	if (UWorld* World = GetWorld())
	{
		UKismetRenderingLibrary::ClearRenderTarget2D(World, RT, FLinearColor::Black);
	}

	return RT;
}

int32 URTPoolManager::FindSlotForCell(FIntPoint CellIndex) const
{
	for (int32 i = 0; i < Pool.Num(); ++i)
	{
		if (Pool[i].AssignedCell == CellIndex)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

int32 URTPoolManager::AssignFreeSlot(FIntPoint CellIndex)
{
	for (int32 i = 0; i < Pool.Num(); ++i)
	{
		if (Pool[i].IsFree())
		{
			FRTPoolEntry& Slot = Pool[i];

			// ImpulseRT may hold stale decay data from the previous occupant.
			// Clear it now before the new invoker starts stamping.
			// ContinuousRT was already cleared eagerly in UnregisterInvoker.
			if (Slot.bImpulseNeedsClear)
			{
				ClearImpulseSlot(i);
				Slot.bImpulseNeedsClear = false;
			}

			Slot.AssignedCell       = CellIndex;
			Slot.CellOriginWS       = CellToWorldOrigin(CellIndex);
			Slot.ActiveInvokerCount = 0;
			Slot.LastReleaseTime    = -1.f;

			UE_LOG(LogTemp, Verbose,
				TEXT("[FoliageRT] Slot %d → Cell (%d,%d)  origin (%.0f, %.0f)"),
				i, CellIndex.X, CellIndex.Y,
				Slot.CellOriginWS.X, Slot.CellOriginWS.Y);

			OnCellAssigned.Broadcast(CellIndex, i);
			return i;
		}
	}
	return INDEX_NONE;   // pool exhausted
}

void URTPoolManager::ClearImpulseSlot(int32 SlotIndex)
{
	if (!Pool.IsValidIndex(SlotIndex))   { return; }
	if (!Pool[SlotIndex].ImpulseRT)      { return; }

	UKismetRenderingLibrary::ClearRenderTarget2D(
		GetWorld(),
		Pool[SlotIndex].ImpulseRT,
		FLinearColor::Black   // G=0 means "never stamped" — foliage skips decay math
	);
}

void URTPoolManager::ClearContinuousSlot(int32 SlotIndex)
{
	if (!Pool.IsValidIndex(SlotIndex))     { return; }
	if (!Pool[SlotIndex].ContinuousRT)     { return; }

	UKismetRenderingLibrary::ClearRenderTarget2D(
		GetWorld(),
		Pool[SlotIndex].ContinuousRT,
		FLinearColor::Black   // A=0 means "no invoker present" — foliage skips static lean
	);
}

void URTPoolManager::ReclaimExpiredSlots()
{
	const float Now = GetWorld()->GetTimeSeconds();

	for (int32 i = 0; i < Pool.Num(); ++i)
	{
		FRTPoolEntry& Slot = Pool[i];

		if (Slot.IsFree())                                 { continue; }
		if (Slot.ActiveInvokerCount > 0)                   { continue; }
		if (Slot.LastReleaseTime < 0.f)                    { continue; }
		if ((Now - Slot.LastReleaseTime) < DecayDuration)  { continue; }

		// Decay window has elapsed — ImpulseRT data is no longer needed.
		// Flag it for a lazy clear on next assignment rather than clearing now.
		const FIntPoint ReclaimedCell = Slot.AssignedCell;

		Slot.bImpulseNeedsClear = true;
		Slot.AssignedCell       = FIntPoint(-1, -1);
		Slot.CellOriginWS       = FVector2D::ZeroVector;
		Slot.LastReleaseTime    = -1.f;

		UE_LOG(LogTemp, Verbose,
			TEXT("[FoliageRT] Slot %d reclaimed from Cell (%d,%d)"),
			i, ReclaimedCell.X, ReclaimedCell.Y);

		OnCellReclaimed.Broadcast(ReclaimedCell, i);
	}
}
