#include "PoolManager/RTPoolManager.h"

#include "Data/RTPoolTypes.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/World.h"

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
		Entry.ImpulseRT    = CreateImpulseRT(FName(*FString::Printf(TEXT("FoliageRT_Impulse_%02d"), i)));
		Entry.ContinuousRT = CreateContinuousRT(FName(*FString::Printf(TEXT("FoliageRT_Continuous_%02d"), i)));
		Pool.Add(Entry);
	}

	UE_LOG(LogTemp, Log,
		TEXT("[FoliageRT] Pool initialised — %d slots, %.0f-unit cells, %dx%d RT, %.1fs decay"),
		PoolSize, CellSize, RTResolution, RTResolution, DecayDuration);
}

void URTPoolManager::Deinitialize()
{
	Pool.Empty();
	Super::Deinitialize();
}

void URTPoolManager::Tick(float DeltaTime)
{
	ReclaimExpiredSlots();
}

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
			UE_LOG(LogTemp, Warning, TEXT("[FoliageRT] Pool fully exhausted — all slots have active invokers. Increase PoolSize."));
			return INDEX_NONE;
		}
	}

	Pool[SlotIdx].ActiveInvokerCount++;
	Pool[SlotIdx].LastReleaseTime = -1.f;
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
		ClearContinuousSlot(SlotIdx);
		Slot.LastReleaseTime = GetWorld()->GetTimeSeconds();
		UE_LOG(LogTemp, Verbose, TEXT("[FoliageRT] Cell (%d,%d) released. Decay clock started — reclaim in %.1fs."), Cell.X, Cell.Y, DecayDuration);
	}
}

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

UTextureRenderTarget2D* URTPoolManager::CreateImpulseRT(const FName& Name) const
{
	UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(GetTransientPackage(), Name);
	RT->RenderTargetFormat = RTF_RGBA32f;
	RT->InitAutoFormat(RTResolution, RTResolution);
	RT->ClearColor = FLinearColor(0.f, 0.f, 0.f, 0.f);
	RT->bAutoGenerateMips = false;
	RT->UpdateResource();
	if (UWorld* World = GetWorld()) { UKismetRenderingLibrary::ClearRenderTarget2D(World, RT, FLinearColor::Black); }
	return RT;
}

UTextureRenderTarget2D* URTPoolManager::CreateContinuousRT(const FName& Name) const
{
	UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(GetTransientPackage(), Name);
	RT->RenderTargetFormat = RTF_RGBA8;
	RT->InitAutoFormat(RTResolution, RTResolution);

	// R=0.5, B=0.5 encodes zero velocity in both axes.
	// A=0 encodes zero weight — foliage ContWeight gate returns 0.
	// Clearing to black (R=0) would decode as -MaxVelocity and shift foliage
	// the moment any cell becomes active, even before a brush stroke is drawn.
	RT->ClearColor        = FLinearColor(0.5f, 0.f, 0.5f, 0.f);
	RT->bAutoGenerateMips = false;
	RT->UpdateResource();

	if (UWorld* World = GetWorld())
	{
		UKismetRenderingLibrary::ClearRenderTarget2D(World, RT, FLinearColor(0.5f, 0.f, 0.5f, 0.f));
	}
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
			UE_LOG(LogTemp, Verbose, TEXT("[FoliageRT] Slot %d → Cell (%d,%d)  origin (%.0f, %.0f)"),
				i, CellIndex.X, CellIndex.Y, Slot.CellOriginWS.X, Slot.CellOriginWS.Y);
			OnCellAssigned.Broadcast(CellIndex, i);
			return i;
		}
	}
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

	if (OldestIdx == INDEX_NONE) { return INDEX_NONE; }

	const FIntPoint ReclaimedCell       = Pool[OldestIdx].AssignedCell;
	Pool[OldestIdx].bImpulseNeedsClear  = true;
	Pool[OldestIdx].AssignedCell        = FIntPoint(INT32_MIN, INT32_MIN);
	Pool[OldestIdx].CellOriginWS        = FVector2D(-9999999.f, -9999999.f);
	Pool[OldestIdx].LastReleaseTime     = -1.f;

	UE_LOG(LogTemp, Verbose, TEXT("[FoliageRT] Slot %d force-evicted from Cell (%d,%d) — age %.1fs"),
		OldestIdx, ReclaimedCell.X, ReclaimedCell.Y, OldestAge);

	OnCellReclaimed.Broadcast(ReclaimedCell, OldestIdx);
	return OldestIdx;
}

void URTPoolManager::ClearImpulseSlot(int32 SlotIndex)
{
	if (!Pool.IsValidIndex(SlotIndex) || !Pool[SlotIndex].ImpulseRT) { return; }
	UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), Pool[SlotIndex].ImpulseRT, FLinearColor::Black);
}

void URTPoolManager::ClearContinuousSlot(int32 SlotIndex)
{
	if (!Pool.IsValidIndex(SlotIndex) || !Pool[SlotIndex].ContinuousRT) { return; }

	// Must clear to neutral velocity (0.5, 0, 0.5, 0) not black.
	// Black decodes as -MaxVelocity and shifts foliage immediately on next sample.
	UKismetRenderingLibrary::ClearRenderTarget2D(
		GetWorld(),
		Pool[SlotIndex].ContinuousRT,
		FLinearColor(0.5f, 0.f, 0.5f, 0.f)
	);
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

		UE_LOG(LogTemp, Verbose, TEXT("[FoliageRT] Slot %d reclaimed from Cell (%d,%d)"),
			i, ReclaimedCell.X, ReclaimedCell.Y);

		OnCellReclaimed.Broadcast(ReclaimedCell, i);
	}
}
