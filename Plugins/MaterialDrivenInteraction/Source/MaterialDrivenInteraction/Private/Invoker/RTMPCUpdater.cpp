// Fill out your copyright notice in the Description page of Project Settings.


#include "Invoker/RTMPCUpdater.h"

#include "PoolManager/RTPoolManager.h"
#include "Data/RTPoolTypes.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Engine/World.h"

// ── Static parameter name constants ──────────────────────────────────────────

const FName URTMPCUpdater::PN_CellSize        (TEXT("FRT_CellSize"));
const FName URTMPCUpdater::PN_ActiveSlotCount (TEXT("FRT_ActiveSlotCount"));

// ── Constructor ───────────────────────────────────────────────────────────────

URTMPCUpdater::URTMPCUpdater()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Tick last in the frame — after the invoker has registered/unregistered,
	// after pool reclaim has run, so the MPC always reflects the settled state.
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

// ── UActorComponent ───────────────────────────────────────────────────────────

void URTMPCUpdater::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World) { return; }

	PoolManager = World->GetSubsystem<URTPoolManager>();
	if (!PoolManager)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RTMPCUpdater] URTPoolManager subsystem not found — component disabled."));
		SetComponentTickEnabled(false);
		return;
	}

	// Bind to pool events so we push textures reactively, not every frame.
	PoolManager->OnCellAssigned.AddDynamic(this, &URTMPCUpdater::OnCellAssigned);
	PoolManager->OnCellReclaimed.AddDynamic(this, &URTMPCUpdater::OnCellReclaimed);

	// Pre-build parameter name arrays up to the actual pool size.
	BuildParameterNames();

	// Seed the MPC with zeroed slot data so materials don't read garbage on frame 1.
	PushVectorDataToMPC();
}

void URTMPCUpdater::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PoolManager)
	{
		PoolManager->OnCellAssigned.RemoveDynamic(this, &URTMPCUpdater::OnCellAssigned);
		PoolManager->OnCellReclaimed.RemoveDynamic(this, &URTMPCUpdater::OnCellReclaimed);
	}

	RegisteredMIDs.Empty();
	Super::EndPlay(EndPlayReason);
}

void URTMPCUpdater::TickComponent(float DeltaTime, ELevelTick TickType,
                                   FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Cell origins can change every frame (slots get reassigned as actors move).
	// Pushing vectors to the MPC is a simple memcpy — no GPU work, very cheap.
	PushVectorDataToMPC();
}

// ── Foliage MID registration ──────────────────────────────────────────────────

void URTMPCUpdater::RegisterFoliageMID(UMaterialInstanceDynamic* MID)
{
	if (!MID) { return; }
	if (RegisteredMIDs.Contains(MID)) { return; }

	RegisteredMIDs.Add(MID);

	// Immediately populate the new MID with current slot state so it doesn't
	// show a frame of stale textures.
	PushAllTexturesToMID(MID);
}

void URTMPCUpdater::UnregisterFoliageMID(UMaterialInstanceDynamic* MID)
{
	RegisteredMIDs.Remove(MID);
}

void URTMPCUpdater::ForceRefreshMID(UMaterialInstanceDynamic* MID)
{
	if (!MID) { return; }
	PushAllTexturesToMID(MID);
}

// ── Utility ───────────────────────────────────────────────────────────────────

UMaterialParameterCollectionInstance* URTMPCUpdater::GetMPCInstance() const
{
	if (!ParameterCollection || !GetWorld()) { return nullptr; }
	return GetWorld()->GetParameterCollectionInstance(ParameterCollection);
}

// ── Internal ──────────────────────────────────────────────────────────────────

void URTMPCUpdater::BuildParameterNames()
{
	const int32 SlotCount = PoolManager
		? FMath::Min(PoolManager->PoolSize, FRT_MAX_POOL_SLOTS)
		: FRT_MAX_POOL_SLOTS;

	ParamName_SlotData.SetNum(SlotCount);
	ParamName_ImpulseRT.SetNum(SlotCount);
	ParamName_ContinuousRT.SetNum(SlotCount);

	for (int32 i = 0; i < SlotCount; ++i)
	{
		ParamName_SlotData[i]     = FName(*FString::Printf(TEXT("FRT_Slot%d_Data"),        i));
		ParamName_ImpulseRT[i]    = FName(*FString::Printf(TEXT("FRT_Slot%d_ImpulseRT"),   i));
		ParamName_ContinuousRT[i] = FName(*FString::Printf(TEXT("FRT_Slot%d_ContinuousRT"),i));
	}
}

void URTMPCUpdater::PushVectorDataToMPC()
{
	if (!ParameterCollection || !PoolManager) { return; }

	UWorld* World = GetWorld();
	if (!World) { return; }

	const TArray<FRTPoolEntry>& Pool = PoolManager->GetPool();
	const int32 SlotCount = FMath::Min(Pool.Num(), ParamName_SlotData.Num());

	int32 ActiveCount = 0;

	for (int32 i = 0; i < SlotCount; ++i)
	{
		const FRTPoolEntry& Slot = Pool[i];
		const bool bActive = Slot.IsOccupied();

		// Pack cell data into a single vector4:
		//   X = CellOriginWS.X
		//   Y = CellOriginWS.Y
		//   Z = 1.0 (occupied) or 0.0 (free)  — material uses Z > 0.5 as the gate
		//   W = slot index as float            — useful for debug material
		const FLinearColor SlotData(
			Slot.CellOriginWS.X,
			Slot.CellOriginWS.Y,
			bActive ? 1.f : 0.f,
			static_cast<float>(i)
		);

		UKismetMaterialLibrary::SetVectorParameterValue(
			World, ParameterCollection, ParamName_SlotData[i], SlotData);

		if (bActive) { ++ActiveCount; }
	}

	// Zero out any slots beyond the current pool size (handles pool resize edge cases)
	for (int32 i = SlotCount; i < ParamName_SlotData.Num(); ++i)
	{
		UKismetMaterialLibrary::SetVectorParameterValue(
			World, ParameterCollection, ParamName_SlotData[i], FLinearColor::Black);
	}

	UKismetMaterialLibrary::SetScalarParameterValue(
		World, ParameterCollection, PN_CellSize,
		PoolManager->CellSize);

	UKismetMaterialLibrary::SetScalarParameterValue(
		World, ParameterCollection, PN_ActiveSlotCount,
		static_cast<float>(ActiveCount));
}

void URTMPCUpdater::PushTexturesForSlot(int32 SlotIndex)
{
	if (!PoolManager) { return; }
	if (!ParamName_ImpulseRT.IsValidIndex(SlotIndex)) { return; }

	const TArray<FRTPoolEntry>& Pool = PoolManager->GetPool();
	if (!Pool.IsValidIndex(SlotIndex)) { return; }

	const FRTPoolEntry& Slot = Pool[SlotIndex];

	// When a slot is reclaimed, Slot.ImpulseRT / ContinuousRT are still valid
	// objects (never destroyed — only reassigned).  Pushing them when the slot
	// is free is harmless; the material gates on FRT_SlotN_Data.Z so it won't
	// produce visible output for inactive slots.
	for (UMaterialInstanceDynamic* MID : RegisteredMIDs)
	{
		if (!MID) { continue; }

		MID->SetTextureParameterValue(ParamName_ImpulseRT[SlotIndex],
			Slot.ImpulseRT);
		MID->SetTextureParameterValue(ParamName_ContinuousRT[SlotIndex],
			Slot.ContinuousRT);
	}
}

void URTMPCUpdater::PushAllTexturesToMID(UMaterialInstanceDynamic* MID)
{
	if (!MID || !PoolManager) { return; }

	const TArray<FRTPoolEntry>& Pool = PoolManager->GetPool();
	const int32 SlotCount = FMath::Min(Pool.Num(), ParamName_ImpulseRT.Num());

	for (int32 i = 0; i < SlotCount; ++i)
	{
		const FRTPoolEntry& Slot = Pool[i];

		MID->SetTextureParameterValue(ParamName_ImpulseRT[i],    Slot.ImpulseRT);
		MID->SetTextureParameterValue(ParamName_ContinuousRT[i], Slot.ContinuousRT);
	}
}

// ── Pool delegate handlers ────────────────────────────────────────────────────

void URTMPCUpdater::OnCellAssigned(FIntPoint CellIndex, int32 SlotIndex)
{
	// A new cell was just assigned — push the fresh RT handles to all MIDs.
	// The MPC vector for this slot will be updated on the next tick automatically.
	PushTexturesForSlot(SlotIndex);

	UE_LOG(LogTemp, Verbose,
		TEXT("[RTMPCUpdater] Slot %d assigned to cell (%d,%d) — textures pushed to %d MIDs"),
		SlotIndex, CellIndex.X, CellIndex.Y, RegisteredMIDs.Num());
}

void URTMPCUpdater::OnCellReclaimed(FIntPoint CellIndex, int32 SlotIndex)
{
	// Slot returned to pool.  The RT objects are still valid (they'll be reused),
	// so no texture push is needed — the material will gate on Z == 0 in the MPC
	// vector for this slot, which PushVectorDataToMPC() will zero out next tick.
	UE_LOG(LogTemp, Verbose,
		TEXT("[RTMPCUpdater] Slot %d reclaimed from cell (%d,%d)"),
		SlotIndex, CellIndex.X, CellIndex.Y);
}
