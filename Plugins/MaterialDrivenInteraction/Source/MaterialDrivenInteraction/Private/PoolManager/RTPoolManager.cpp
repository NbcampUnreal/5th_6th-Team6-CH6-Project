#include "PoolManager/RTPoolManager.h"

#include "Data/RTPoolTypes.h"
#include "Debug/LogCategory.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/World.h"

static const TCHAR* RTFormatName(ETextureRenderTargetFormat Fmt)
{
	switch (Fmt)
	{
		case RTF_R8:      return TEXT("RTF_R8");
		case RTF_RGBA8:   return TEXT("RTF_RGBA8");
		case RTF_RGBA16f: return TEXT("RTF_RGBA16f");
		case RTF_RGBA32f: return TEXT("RTF_RGBA32f");
		default:          return TEXT("RTF_Unknown");
	}
}

// Neutral clear for ContinuousRT — used in multiple places, defined once.
static const FLinearColor GContinuousNeutral(0.5f, 0.5f, 0.f, 0.f);

bool URTPoolManager::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	return World && (World->IsGameWorld() || World->IsPlayInEditor());
}

void URTPoolManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (const URTPoolSettings* Settings = GetDefault<URTPoolSettings>())
	{
		PoolSize      = FMath::Clamp(Settings->PoolSize,      1,    16);
		CellSize      = FMath::Max  (Settings->CellSize,      100.f);
		RTResolution  = FMath::Clamp(Settings->RTResolution,  64,  2048);
		DecayDuration = FMath::Max  (Settings->DecayDuration,  0.f);
	}

	Pool.Reserve(PoolSize);

	for (int32 i = 0; i < PoolSize; ++i)
	{
		FRTPoolEntry Entry;
		Entry.ImpulseRT    = LoadImpulseRT(i);
		Entry.ContinuousRT = LoadContinuousRT(i);
		Pool.Add(Entry);
	}

	UE_LOG(RTFoliageInvoker, Log,
		TEXT("URTPoolManager::Initialize >> Pool ready — %d slots, %.0f-unit cells, %dx%d RT, %.1fs decay"),
		PoolSize, CellSize, RTResolution, RTResolution, DecayDuration);
}

void URTPoolManager::Deinitialize()
{
	UE_LOG(RTFoliageInvoker, Log,
		TEXT("URTPoolManager::Deinitialize >> Releasing %d pool slots"),
		Pool.Num());

	Pool.Empty();
	Super::Deinitialize();
}

void URTPoolManager::Tick(float DeltaTime)
{
	// Frame guard — Tick is called by every invoker component each frame.
	// Only execute once per frame regardless of how many invokers exist.
	const uint64 CurrentFrame = GFrameCounter;
	if (CurrentFrame == LastTickFrame) { return; }
	LastTickFrame = CurrentFrame;

	// Clear all occupied ContinuousRTs once at the start of each frame.
	// This must happen before any invoker draws so all invoker brushes
	// accumulate additively into a fresh RT — no trails, no cross-invoker bleed.
	for (int32 i = 0; i < Pool.Num(); ++i)
	{
		if (Pool[i].IsOccupied() && Pool[i].ContinuousRT)
		{
			UKismetRenderingLibrary::ClearRenderTarget2D(
				GetWorld(), Pool[i].ContinuousRT, GContinuousNeutral);
		}
	}

	ReclaimExpiredSlots();
}

// ── Invoker API ───────────────────────────────────────────────────────────────

int32 URTPoolManager::RegisterInvoker(FVector WorldLocation)
{
	const FIntPoint Cell = WorldToCell(WorldLocation);
	int32 SlotIdx = FindSlotForCell(Cell);

	if (SlotIdx == INDEX_NONE)
	{
		SlotIdx = AssignFreeSlot(Cell);

		if (SlotIdx == INDEX_NONE)
		{
			const int32 EvictedIdx = EvictOldestReleasedSlot();
			if (EvictedIdx != INDEX_NONE)
			{
				SlotIdx = AssignFreeSlot(Cell);
			}
		}

		if (SlotIdx == INDEX_NONE)
		{
			UE_LOG(RTFoliageInvoker, Warning,
				TEXT("URTPoolManager::RegisterInvoker >> Pool exhausted — all %d slots have active invokers. Increase PoolSize."),
				PoolSize);
			return INDEX_NONE;
		}
	}

	Pool[SlotIdx].ActiveInvokerCount++;
	Pool[SlotIdx].LastReleaseTime = -1.f;

	UE_LOG(RTFoliageInvoker, Verbose,
		TEXT("URTPoolManager::RegisterInvoker >> Cell (%d,%d) → Slot %d (invokers: %d)"),
		Cell.X, Cell.Y, SlotIdx, Pool[SlotIdx].ActiveInvokerCount);

	return SlotIdx;
}

void URTPoolManager::UnregisterInvoker(FVector WorldLocation)
{
	const FIntPoint Cell    = WorldToCell(WorldLocation);
	const int32     SlotIdx = FindSlotForCell(Cell);
	if (SlotIdx == INDEX_NONE) { return; }

	FRTPoolEntry& Slot      = Pool[SlotIdx];
	Slot.ActiveInvokerCount = FMath::Max(0, Slot.ActiveInvokerCount - 1);

	if (Slot.ActiveInvokerCount == 0)
	{
		// Eagerly clear ContinuousRT when last invoker leaves — no brush present.
		ClearContinuousSlot(SlotIdx);
		Slot.LastReleaseTime = GetWorld()->GetTimeSeconds();

		UE_LOG(RTFoliageInvoker, Verbose,
			TEXT("URTPoolManager::UnregisterInvoker >> Cell (%d,%d) released — decay clock started, reclaim in %.1fs"),
			Cell.X, Cell.Y, DecayDuration);
	}
	else
	{
		UE_LOG(RTFoliageInvoker, Verbose,
			TEXT("URTPoolManager::UnregisterInvoker >> Cell (%d,%d) Slot %d — invokers remaining: %d"),
			Cell.X, Cell.Y, SlotIdx, Slot.ActiveInvokerCount);
	}
}

// ── Query API ─────────────────────────────────────────────────────────────────

bool URTPoolManager::GetRTForWorldPosition(FVector WorldLocation,
	UTextureRenderTarget2D*& OutImpulseRT, UTextureRenderTarget2D*& OutContinuousRT, FVector2D& OutUV) const
{
	const FIntPoint Cell    = WorldToCell(WorldLocation);
	const int32     SlotIdx = FindSlotForCell(Cell);

	if (SlotIdx == INDEX_NONE)
	{
		OutImpulseRT = nullptr; OutContinuousRT = nullptr; OutUV = FVector2D::ZeroVector;
		return false;
	}

	const FRTPoolEntry& Slot = Pool[SlotIdx];
	OutImpulseRT    = Slot.ImpulseRT;
	OutContinuousRT = Slot.ContinuousRT;
	const FVector2D WorldXY(WorldLocation.X, WorldLocation.Y);
	OutUV = (WorldXY - Slot.CellOriginWS) / CellSize;
	OutUV = OutUV.ClampAxes(0.f, 1.f);
	return true;
}

bool URTPoolManager::GetSlotData(int32 SlotIndex,
	UTextureRenderTarget2D*& OutImpulseRT, UTextureRenderTarget2D*& OutContinuousRT, FVector2D& OutCellOriginWS) const
{
	if (!Pool.IsValidIndex(SlotIndex))
	{
		OutImpulseRT = nullptr; OutContinuousRT = nullptr; OutCellOriginWS = FVector2D::ZeroVector;
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
	return FVector2D(CellIndex.X * CellSize, CellIndex.Y * CellSize);
}

int32 URTPoolManager::GetActiveSlotCount() const
{
	int32 Count = 0;
	for (const FRTPoolEntry& Slot : Pool) { if (Slot.IsOccupied()) { ++Count; } }
	return Count;
}

FString URTPoolManager::GetPoolDebugString() const
{
	FString Out;
	Out.Reserve(512);

	Out += FString::Printf(
		TEXT("=== RTPoolManager | Slots:%d CellSize:%.0f DecayDuration:%.1fs ===\n"),
		PoolSize, CellSize, DecayDuration);

	for (int32 i = 0; i < Pool.Num(); ++i)
	{
		const FRTPoolEntry& Slot = Pool[i];

		if (Slot.IsFree())
		{
			Out += FString::Printf(TEXT("[Slot %02d] FREE\n"), i);
			continue;
		}

		const FString LocationStr = FString::Printf(
			TEXT("Cell(%d,%d)  Origin(%.0f, %.0f)"),
			Slot.AssignedCell.X, Slot.AssignedCell.Y,
			Slot.CellOriginWS.X, Slot.CellOriginWS.Y);

		FString DecayStr;
		if (Slot.ActiveInvokerCount > 0)
		{
			DecayStr = FString::Printf(TEXT("Invokers:%d  [ACTIVE]"), Slot.ActiveInvokerCount);
		}
		else
		{
			const float Elapsed = (GetWorld() && Slot.LastReleaseTime >= 0.f)
				? GetWorld()->GetTimeSeconds() - Slot.LastReleaseTime : 0.f;
			DecayStr = FString::Printf(
				TEXT("Invokers:0  [DECAYING]  Elapsed:%.1fs  Remaining:%.1fs"),
				Elapsed, FMath::Max(0.f, DecayDuration - Elapsed));
		}

		const FString ImpStr = Slot.ImpulseRT
			? FString::Printf(TEXT("ImpulseRT:   '%s'  %dx%d  %s  NeedsClear:%s"),
				*Slot.ImpulseRT->GetName(),
				Slot.ImpulseRT->SizeX, Slot.ImpulseRT->SizeY,
				RTFormatName(Slot.ImpulseRT->RenderTargetFormat),
				Slot.bImpulseNeedsClear ? TEXT("YES") : TEXT("no"))
			: TEXT("ImpulseRT:   NULL");

		const FString ContStr = Slot.ContinuousRT
			? FString::Printf(TEXT("ContinuousRT:'%s'  %dx%d  %s"),
				*Slot.ContinuousRT->GetName(),
				Slot.ContinuousRT->SizeX, Slot.ContinuousRT->SizeY,
				RTFormatName(Slot.ContinuousRT->RenderTargetFormat))
			: TEXT("ContinuousRT:NULL");

		Out += FString::Printf(
			TEXT("[Slot %02d] %s  |  %s\n")
			TEXT("          %s\n")
			TEXT("          %s\n"),
			i, *LocationStr, *DecayStr, *ImpStr, *ContStr);
	}

	return Out;
}

// ── Private helpers ───────────────────────────────────────────────────────────

UTextureRenderTarget2D* URTPoolManager::LoadImpulseRT(int32 SlotIndex) const
{
	const URTPoolSettings* Settings = GetDefault<URTPoolSettings>();
	if (!Settings || !Settings->ImpulseRTs.IsValidIndex(SlotIndex))
	{
		UE_LOG(RTFoliageInvoker, Error,
			TEXT("URTPoolManager::LoadImpulseRT >> No ImpulseRT asset assigned for Slot %d — assign in Project Settings → Foliage RT Pool"),
			SlotIndex);
		return nullptr;
	}

	UTextureRenderTarget2D* RT = Settings->ImpulseRTs[SlotIndex].LoadSynchronous();
	if (!RT)
	{
		UE_LOG(RTFoliageInvoker, Error,
			TEXT("URTPoolManager::LoadImpulseRT >> Failed to load ImpulseRT asset for Slot %d"),
			SlotIndex);
		return nullptr;
	}

	if (UWorld* World = GetWorld())
	{
		// Black clear — A=0 ensures FRT_DampedSine guard fires immediately,
		// preventing phantom wobble before any invoker stamps.
		UKismetRenderingLibrary::ClearRenderTarget2D(World, RT, FLinearColor::Black);
	}

	UE_LOG(RTFoliageInvoker, Log,
		TEXT("URTPoolManager::LoadImpulseRT >> Loaded '%s' for Slot %d"),
		*RT->GetName(), SlotIndex);

	return RT;
}

UTextureRenderTarget2D* URTPoolManager::LoadContinuousRT(int32 SlotIndex) const
{
	const URTPoolSettings* Settings = GetDefault<URTPoolSettings>();
	if (!Settings || !Settings->ContinuousRTs.IsValidIndex(SlotIndex))
	{
		UE_LOG(RTFoliageInvoker, Error,
			TEXT("URTPoolManager::LoadContinuousRT >> No ContinuousRT asset assigned for Slot %d — assign in Project Settings → Foliage RT Pool"),
			SlotIndex);
		return nullptr;
	}

	UTextureRenderTarget2D* RT = Settings->ContinuousRTs[SlotIndex].LoadSynchronous();
	if (!RT)
	{
		UE_LOG(RTFoliageInvoker, Error,
			TEXT("URTPoolManager::LoadContinuousRT >> Failed to load ContinuousRT asset for Slot %d"),
			SlotIndex);
		return nullptr;
	}

	if (UWorld* World = GetWorld())
	{
		// (0.5, 0.5, 0, 0) — neutral direction, height unused, no weight.
		UKismetRenderingLibrary::ClearRenderTarget2D(World, RT, GContinuousNeutral);
	}

	UE_LOG(RTFoliageInvoker, Log,
		TEXT("URTPoolManager::LoadContinuousRT >> Loaded '%s' for Slot %d"),
		*RT->GetName(), SlotIndex);

	return RT;
}

int32 URTPoolManager::FindSlotForCell(FIntPoint CellIndex) const
{
	for (int32 i = 0; i < Pool.Num(); ++i) { if (Pool[i].AssignedCell == CellIndex) { return i; } }
	return INDEX_NONE;
}

int32 URTPoolManager::AssignFreeSlot(FIntPoint CellIndex)
{
	for (int32 i = 0; i < Pool.Num(); ++i)
	{
		if (Pool[i].IsFree())
		{
			FRTPoolEntry& Slot = Pool[i];
			if (Slot.bImpulseNeedsClear) { ClearImpulseSlot(i); Slot.bImpulseNeedsClear = false; }
			Slot.AssignedCell       = CellIndex;
			Slot.CellOriginWS       = CellToWorldOrigin(CellIndex);
			Slot.ActiveInvokerCount = 0;
			Slot.LastReleaseTime    = -1.f;

			UE_LOG(RTFoliageInvoker, Verbose,
				TEXT("URTPoolManager::AssignFreeSlot >> Slot %d → Cell (%d,%d) origin (%.0f,%.0f)"),
				i, CellIndex.X, CellIndex.Y, Slot.CellOriginWS.X, Slot.CellOriginWS.Y);

			OnCellAssigned.Broadcast(CellIndex, i);
			return i;
		}
	}

	UE_LOG(RTFoliageInvoker, Verbose,
		TEXT("URTPoolManager::AssignFreeSlot >> No free slot available for Cell (%d,%d)"),
		CellIndex.X, CellIndex.Y);

	return INDEX_NONE;
}

int32 URTPoolManager::EvictOldestReleasedSlot()
{
	int32 OldestIdx = INDEX_NONE;
	float OldestAge = -1.f;
	const float Now = GetWorld()->GetTimeSeconds();

	for (int32 i = 0; i < Pool.Num(); ++i)
	{
		const FRTPoolEntry& Slot = Pool[i];
		if (Slot.IsFree() || Slot.ActiveInvokerCount > 0 || Slot.LastReleaseTime < 0.f) { continue; }
		const float Age = Now - Slot.LastReleaseTime;
		if (Age > OldestAge) { OldestAge = Age; OldestIdx = i; }
	}

	if (OldestIdx == INDEX_NONE)
	{
		UE_LOG(RTFoliageInvoker, Warning,
			TEXT("URTPoolManager::EvictOldestReleasedSlot >> No evictable slot found — all slots have active invokers"));
		return INDEX_NONE;
	}

	const FIntPoint ReclaimedCell      = Pool[OldestIdx].AssignedCell;
	Pool[OldestIdx].bImpulseNeedsClear = true;
	Pool[OldestIdx].AssignedCell       = FIntPoint(INT32_MIN, INT32_MIN);
	Pool[OldestIdx].CellOriginWS       = FVector2D(-9999999.f, -9999999.f);
	Pool[OldestIdx].LastReleaseTime    = -1.f;

	UE_LOG(RTFoliageInvoker, Log,
		TEXT("URTPoolManager::EvictOldestReleasedSlot >> Slot %d force-evicted from Cell (%d,%d) — age %.1fs"),
		OldestIdx, ReclaimedCell.X, ReclaimedCell.Y, OldestAge);

	OnCellReclaimed.Broadcast(ReclaimedCell, OldestIdx);
	return OldestIdx;
}

void URTPoolManager::ClearImpulseSlot(int32 SlotIndex)
{
	if (!Pool.IsValidIndex(SlotIndex) || !Pool[SlotIndex].ImpulseRT) { return; }
	UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), Pool[SlotIndex].ImpulseRT, FLinearColor::Black);

	UE_LOG(RTFoliageInvoker, Verbose,
		TEXT("URTPoolManager::ClearImpulseSlot >> Slot %d ImpulseRT cleared to black"),
		SlotIndex);
}

void URTPoolManager::ClearContinuousSlot(int32 SlotIndex)
{
	if (!Pool.IsValidIndex(SlotIndex) || !Pool[SlotIndex].ContinuousRT) { return; }

	UKismetRenderingLibrary::ClearRenderTarget2D(
		GetWorld(), Pool[SlotIndex].ContinuousRT, GContinuousNeutral);

	UE_LOG(RTFoliageInvoker, Verbose,
		TEXT("URTPoolManager::ClearContinuousSlot >> Slot %d ContinuousRT cleared to neutral"),
		SlotIndex);
}

void URTPoolManager::ReclaimExpiredSlots()
{
	const float Now = GetWorld()->GetTimeSeconds();

	for (int32 i = 0; i < Pool.Num(); ++i)
	{
		FRTPoolEntry& Slot = Pool[i];
		if (Slot.IsFree() || Slot.ActiveInvokerCount > 0 || Slot.LastReleaseTime < 0.f) { continue; }
		if ((Now - Slot.LastReleaseTime) < DecayDuration) { continue; }

		const FIntPoint ReclaimedCell = Slot.AssignedCell;
		Slot.bImpulseNeedsClear = true;
		Slot.AssignedCell       = FIntPoint(INT32_MIN, INT32_MIN);
		Slot.CellOriginWS       = FVector2D(-9999999.f, -9999999.f);
		Slot.LastReleaseTime    = -1.f;

		UE_LOG(RTFoliageInvoker, Verbose,
			TEXT("URTPoolManager::ReclaimExpiredSlots >> Slot %d reclaimed from Cell (%d,%d)"),
			i, ReclaimedCell.X, ReclaimedCell.Y);

		OnCellReclaimed.Broadcast(ReclaimedCell, i);
	}
}