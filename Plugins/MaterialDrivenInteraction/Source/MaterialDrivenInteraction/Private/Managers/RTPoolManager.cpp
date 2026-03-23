#include "Managers/RTPoolManager.h"

#include "Data/RTPoolTypes.h"
#include "Invoker/FoliageRTInvokerComponent.h"
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
	RegisteredInvokers.Empty();
	Pool.Empty();

	UE_LOG(RTFoliageInvoker, Log, TEXT("URTPoolManager::Deinitialize >> Done"));
	Super::Deinitialize();
}

void URTPoolManager::Tick(float DeltaTime)
{
	const uint64 CurrentFrame = GFrameCounter;
	if (CurrentFrame == LastTickFrame) { return; }
	LastTickFrame = CurrentFrame;
}

void URTPoolManager::SetPriorityCenter(FVector WorldLocation)
{
	PriorityCenter = WorldLocation;
}

// ── Invoker registration ──────────────────────────────────────────────────────

void URTPoolManager::RegisterInvoker(UFoliageRTInvokerComponent* Invoker)
{
	if (!Invoker) { return; }
	RegisteredInvokers.AddUnique(Invoker);

	UE_LOG(RTFoliageInvoker, Log,
		TEXT("URTPoolManager::RegisterInvoker >> Registered — total invokers: %d"),
		RegisteredInvokers.Num());
}

void URTPoolManager::UnregisterInvoker(UFoliageRTInvokerComponent* Invoker)
{
	if (!Invoker) { return; }
	RegisteredInvokers.RemoveSwap(Invoker);

	UE_LOG(RTFoliageInvoker, Log,
		TEXT("URTPoolManager::UnregisterInvoker >> Unregistered — total invokers: %d"),
		RegisteredInvokers.Num());
}

// ── Slot evaluation ───────────────────────────────────────────────────────────

TArray<TTuple<UFoliageRTInvokerComponent*, int32>> URTPoolManager::EvaluateAndAssignSlots()
{
	TArray<TTuple<UFoliageRTInvokerComponent*, int32>> Result;

	RegisteredInvokers.RemoveAll([](const TWeakObjectPtr<UFoliageRTInvokerComponent>& W)
	{
		return !W.IsValid();
	});

	if (RegisteredInvokers.Num() == 0) { return Result; }

	const FVector2D PriorityCenter2D(PriorityCenter.X, PriorityCenter.Y);

	// Sort invokers by distance from PriorityCenter — closest first
	TArray<TPair<float, UFoliageRTInvokerComponent*>> Sorted;
	Sorted.Reserve(RegisteredInvokers.Num());

	for (const TWeakObjectPtr<UFoliageRTInvokerComponent>& WeakInvoker : RegisteredInvokers)
	{
		UFoliageRTInvokerComponent* Invoker = WeakInvoker.Get();
		if (!Invoker) { continue; }

		const FVector  Loc  = Invoker->GetComponentLocation();
		const float    Dist = FVector2D::Distance(PriorityCenter2D, FVector2D(Loc.X, Loc.Y));
		Sorted.Add(TPair<float, UFoliageRTInvokerComponent*>(Dist, Invoker));
	}

	Sorted.Sort([](const TPair<float, UFoliageRTInvokerComponent*>& A,
	               const TPair<float, UFoliageRTInvokerComponent*>& B)
	{
		return A.Key < B.Key;
	});

	// Release all current slot assignments — fresh evaluation each cycle
	for (FRTPoolEntry& Slot : Pool)
	{
		if (Slot.IsOccupied())
		{
			Slot.ActiveInvokerCount = 0;
			if (Slot.LastReleaseTime < 0.f)
			{
				Slot.LastReleaseTime = GetWorld()->GetTimeSeconds();
			}
		}
	}

	int32 AssignedCount = 0;

	for (const TPair<float, UFoliageRTInvokerComponent*>& Pair : Sorted)
	{
		UFoliageRTInvokerComponent* Invoker = Pair.Value;
		const FVector               Loc     = Invoker->GetComponentLocation();
		const float                 Radius  = Invoker->BrushRadius;
		const FVector2D             Centre(Loc.X, Loc.Y);

		// Find all cells overlapping brush radius
		const FIntPoint MinCell = WorldToCell(FVector(Centre.X - Radius, Centre.Y - Radius, 0.f));
		const FIntPoint MaxCell = WorldToCell(FVector(Centre.X + Radius, Centre.Y + Radius, 0.f));

		for (int32 CellX = MinCell.X; CellX <= MaxCell.X; ++CellX)
		{
			for (int32 CellY = MinCell.Y; CellY <= MaxCell.Y; ++CellY)
			{
				if (AssignedCount >= PoolSize) { break; }

				const FIntPoint Cell(CellX, CellY);

				// Find existing slot for this cell
				int32 SlotIdx = INDEX_NONE;
				for (int32 i = 0; i < Pool.Num(); ++i)
				{
					if (Pool[i].AssignedCell == Cell)
					{
						SlotIdx = i;
						break;
					}
				}

				if (SlotIdx == INDEX_NONE)
				{
					SlotIdx = AssignFreeSlot();
					if (SlotIdx == INDEX_NONE)
					{
						SlotIdx = EvictBestSlot();
					}

					if (SlotIdx != INDEX_NONE)
					{
						Pool[SlotIdx].AssignedCell    = Cell;
						Pool[SlotIdx].CellOriginWS    = CellToWorldOrigin(Cell);
						Pool[SlotIdx].LastReleaseTime = -1.f;
						OnCellAssigned.Broadcast(Cell, SlotIdx);
					}
				}

				if (SlotIdx == INDEX_NONE)
				{
					UE_LOG(RTFoliageInvoker, Warning,
						TEXT("URTPoolManager::EvaluateAndAssignSlots >> No slot for Cell(%d,%d)"),
						CellX, CellY);
					continue;
				}

				Pool[SlotIdx].ActiveInvokerCount = 1;
				Pool[SlotIdx].LastReleaseTime    = -1.f;

				Result.Add(TTuple<UFoliageRTInvokerComponent*, int32>(Invoker, SlotIdx));
				++AssignedCount;
			}
		}
	}

	UE_LOG(RTFoliageInvoker, Verbose,
		TEXT("URTPoolManager::EvaluateAndAssignSlots >> Assigned %d slot-invoker pairs"),
		AssignedCount);

	return Result;
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
		Slot.bImpulseNeedsClear       = true;
		Slot.AssignedCell             = FIntPoint(INT32_MIN, INT32_MIN);
		Slot.CellOriginWS             = FVector2D(-9999999.f, -9999999.f);
		Slot.LastReleaseTime          = -1.f;

		UE_LOG(RTFoliageInvoker, Verbose,
			TEXT("URTPoolManager::ReclaimExpiredSlots >> Slot %d reclaimed from Cell (%d,%d)"),
			i, ReclaimedCell.X, ReclaimedCell.Y);

		OnCellReclaimed.Broadcast(ReclaimedCell, i);
	}
}

void URTPoolManager::ClearImpulseFlag(int32 SlotIndex)
{
	if (Pool.IsValidIndex(SlotIndex))
	{
		Pool[SlotIndex].bImpulseNeedsClear = false;
	}
}

// ── Query API ─────────────────────────────────────────────────────────────────

bool URTPoolManager::GetRTForWorldPosition(FVector WorldLocation,
	UTextureRenderTarget2D*& OutImpulseRT, UTextureRenderTarget2D*& OutContinuousRT, FVector2D& OutUV) const
{
	const FIntPoint Cell = WorldToCell(WorldLocation);

	for (int32 i = 0; i < Pool.Num(); ++i)
	{
		if (Pool[i].AssignedCell == Cell)
		{
			OutImpulseRT    = Pool[i].ImpulseRT;
			OutContinuousRT = Pool[i].ContinuousRT;
			const FVector2D WorldXY(WorldLocation.X, WorldLocation.Y);
			OutUV = (WorldXY - Pool[i].CellOriginWS) / CellSize;
			OutUV = OutUV.ClampAxes(0.f, 1.f);
			return true;
		}
	}

	OutImpulseRT = nullptr; OutContinuousRT = nullptr; OutUV = FVector2D::ZeroVector;
	return false;
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
		TEXT("=== RTPoolManager | Slots:%d CellSize:%.0f DecayDuration:%.1fs Invokers:%d PriorityCenter:(%.0f,%.0f) ===\n"),
		PoolSize, CellSize, DecayDuration, RegisteredInvokers.Num(),
		PriorityCenter.X, PriorityCenter.Y);

	for (int32 i = 0; i < Pool.Num(); ++i)
	{
		const FRTPoolEntry& Slot = Pool[i];

		if (Slot.IsFree())
		{
			Out += FString::Printf(TEXT("[Slot %02d] FREE\n"), i);
			continue;
		}

		const float DistFromPriority = FVector2D::Distance(
			FVector2D(PriorityCenter.X, PriorityCenter.Y),
			Slot.CellOriginWS + FVector2D(CellSize * 0.5f));

		FString DecayStr;
		if (Slot.ActiveInvokerCount > 0)
		{
			DecayStr = FString::Printf(TEXT("Invokers:%d  [ACTIVE]  Dist:%.0f"),
				Slot.ActiveInvokerCount, DistFromPriority);
		}
		else
		{
			const float Elapsed = (GetWorld() && Slot.LastReleaseTime >= 0.f)
				? GetWorld()->GetTimeSeconds() - Slot.LastReleaseTime : 0.f;
			DecayStr = FString::Printf(
				TEXT("Invokers:0  [DECAYING]  Elapsed:%.1fs  Remaining:%.1fs  Dist:%.0f"),
				Elapsed, FMath::Max(0.f, DecayDuration - Elapsed), DistFromPriority);
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
			TEXT("[Slot %02d] Cell(%d,%d) Origin(%.0f,%.0f)  |  %s\n")
			TEXT("          %s\n")
			TEXT("          %s\n"),
			i, Slot.AssignedCell.X, Slot.AssignedCell.Y,
			Slot.CellOriginWS.X, Slot.CellOriginWS.Y,
			*DecayStr, *ImpStr, *ContStr);
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
			TEXT("URTPoolManager::LoadImpulseRT >> No ImpulseRT assigned for Slot %d"), SlotIndex);
		return nullptr;
	}

	UTextureRenderTarget2D* RT = Settings->ImpulseRTs[SlotIndex].LoadSynchronous();
	if (!RT)
	{
		UE_LOG(RTFoliageInvoker, Error,
			TEXT("URTPoolManager::LoadImpulseRT >> Failed to load Slot %d"), SlotIndex);
		return nullptr;
	}

	if (UWorld* World = GetWorld())
	{
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
			TEXT("URTPoolManager::LoadContinuousRT >> No ContinuousRT assigned for Slot %d"), SlotIndex);
		return nullptr;
	}

	UTextureRenderTarget2D* RT = Settings->ContinuousRTs[SlotIndex].LoadSynchronous();
	if (!RT)
	{
		UE_LOG(RTFoliageInvoker, Error,
			TEXT("URTPoolManager::LoadContinuousRT >> Failed to load Slot %d"), SlotIndex);
		return nullptr;
	}

	if (UWorld* World = GetWorld())
	{
		UKismetRenderingLibrary::ClearRenderTarget2D(World, RT,
			FLinearColor(0.5f, 0.5f, 0.f, 0.f));
	}

	UE_LOG(RTFoliageInvoker, Log,
		TEXT("URTPoolManager::LoadContinuousRT >> Loaded '%s' for Slot %d"),
		*RT->GetName(), SlotIndex);

	return RT;
}

int32 URTPoolManager::AssignFreeSlot()
{
	for (int32 i = 0; i < Pool.Num(); ++i)
	{
		if (Pool[i].IsFree())
		{
			FRTPoolEntry& Slot      = Pool[i];
			Slot.ActiveInvokerCount = 0;
			Slot.LastReleaseTime    = -1.f;

			// ImpulseRT clear is handled by URTDrawManager via bImpulseNeedsClear flag
			UE_LOG(RTFoliageInvoker, Verbose,
				TEXT("URTPoolManager::AssignFreeSlot >> Free slot %d found"), i);
			return i;
		}
	}
	return INDEX_NONE;
}

int32 URTPoolManager::EvictBestSlot()
{
	const FVector2D PriorityCenter2D(PriorityCenter.X, PriorityCenter.Y);
	const float     Now = GetWorld()->GetTimeSeconds();

	int32 BestIdx    = INDEX_NONE;
	float BestDist   = -1.f;
	float BestAge    = -1.f;
	bool  bFoundFree = false;

	for (int32 i = 0; i < Pool.Num(); ++i)
	{
		const FRTPoolEntry& Slot = Pool[i];
		if (Slot.IsFree()) { continue; }

		const FVector2D SlotCenter = Slot.CellOriginWS + FVector2D(CellSize * 0.5f);
		const float     Dist       = FVector2D::Distance(PriorityCenter2D, SlotCenter);
		const float     Age        = Slot.LastReleaseTime >= 0.f
		                             ? Now - Slot.LastReleaseTime : -1.f;
		const bool      bReleased  = Slot.ActiveInvokerCount == 0;

		if (bReleased && !bFoundFree)
		{
			bFoundFree = true;
			BestIdx = i; BestDist = Dist; BestAge = Age;
		}
		else if (bReleased && bFoundFree)
		{
			if (Dist > BestDist || (FMath::IsNearlyEqual(Dist, BestDist) && Age > BestAge))
			{
				BestIdx = i; BestDist = Dist; BestAge = Age;
			}
		}
		else if (!bReleased && !bFoundFree)
		{
			if (Dist > BestDist) { BestIdx = i; BestDist = Dist; }
		}
	}

	if (BestIdx == INDEX_NONE)
	{
		UE_LOG(RTFoliageInvoker, Warning,
			TEXT("URTPoolManager::EvictBestSlot >> No evictable slot found"));
		return INDEX_NONE;
	}

	const FIntPoint ReclaimedCell    = Pool[BestIdx].AssignedCell;
	Pool[BestIdx].bImpulseNeedsClear = true;
	Pool[BestIdx].AssignedCell       = FIntPoint(INT32_MIN, INT32_MIN);
	Pool[BestIdx].CellOriginWS       = FVector2D(-9999999.f, -9999999.f);
	Pool[BestIdx].LastReleaseTime    = -1.f;
	Pool[BestIdx].ActiveInvokerCount = 0;

	UE_LOG(RTFoliageInvoker, Log,
		TEXT("URTPoolManager::EvictBestSlot >> Slot %d evicted from Cell (%d,%d) — dist=%.0f released=%s"),
		BestIdx, ReclaimedCell.X, ReclaimedCell.Y, BestDist,
		bFoundFree ? TEXT("yes") : TEXT("no"));

	OnCellReclaimed.Broadcast(ReclaimedCell, BestIdx);
	return BestIdx;
}