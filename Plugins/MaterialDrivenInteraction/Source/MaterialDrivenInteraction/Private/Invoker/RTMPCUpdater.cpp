
#include "Invoker/RTMPCUpdater.h"

#include "PoolManager/RTPoolManager.h"
#include "Data/RTPoolTypes.h"
#include "Debug/LogCategory.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

// ── Static parameter name constants ──────────────────────────────────────────

const FName URTMPCUpdater::PN_CellSize        (TEXT("FRT_CellSize"));
const FName URTMPCUpdater::PN_ActiveSlotCount (TEXT("FRT_ActiveSlotCount"));

// ── Constructor ───────────────────────────────────────────────────────────────

URTMPCUpdater::URTMPCUpdater()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup   = TG_PostUpdateWork;
}

// ── UActorComponent ───────────────────────────────────────────────────────────

void URTMPCUpdater::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetOwner() ? GetOwner()->GetWorld() : nullptr;
	if (!World)
	{
		UE_LOG(RTFoliageInvoker, Warning,
			TEXT("URTMPCUpdater::BeginPlay >> World is null"));
		return;
	}

	PoolManager = World->GetSubsystem<URTPoolManager>();
	if (!PoolManager)
	{
		UE_LOG(RTFoliageInvoker, Warning,
			TEXT("URTMPCUpdater::BeginPlay >> URTPoolManager subsystem not found — component disabled"));
		SetComponentTickEnabled(false);
		return;
	}

	PoolManager->OnCellAssigned.AddDynamic(this, &URTMPCUpdater::OnCellAssigned);
	PoolManager->OnCellReclaimed.AddDynamic(this, &URTMPCUpdater::OnCellReclaimed);

	BuildParameterNames();
	PushVectorDataToMPC();

	UE_LOG(RTFoliageInvoker, Log,
		TEXT("URTMPCUpdater::BeginPlay >> Initialised — PoolSize=%d CellSize=%.0f"),
		PoolManager->PoolSize, PoolManager->CellSize);
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
	PushVectorDataToMPC();

	// Mirror pool into DebugPool so Details panel shows RT thumbnails
	if (PoolManager)
	{
		DebugPool = PoolManager->GetPool();
	}
}

// ── Foliage MID registration ──────────────────────────────────────────────────

void URTMPCUpdater::RegisterFoliageMID(UMaterialInstanceDynamic* MID)
{
	if (!MID) { return; }
	if (RegisteredMIDs.Contains(MID)) { return; }

	RegisteredMIDs.Add(MID);
	PushAllTexturesToMID(MID);

	UE_LOG(RTFoliageInvoker, Log,
		TEXT("URTMPCUpdater::RegisterFoliageMID >> MID registered — total MIDs: %d"),
		RegisteredMIDs.Num());
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
	if (!ParameterCollection || !GetOwner() || !GetOwner()->GetWorld()) { return nullptr; }
	return GetOwner()->GetWorld()->GetParameterCollectionInstance(ParameterCollection);
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
		ParamName_SlotData[i]     = FName(*FString::Printf(TEXT("FRT_Slot%d_Data"),         i));
		ParamName_ImpulseRT[i]    = FName(*FString::Printf(TEXT("FRT_Slot%d_ImpulseRT"),    i));
		ParamName_ContinuousRT[i] = FName(*FString::Printf(TEXT("FRT_Slot%d_ContinuousRT"), i));
	}
}

void URTMPCUpdater::PushVectorDataToMPC()
{
	if (!ParameterCollection || !PoolManager) { return; }

	UWorld* World = GetOwner() ? GetOwner()->GetWorld() : nullptr;
	if (!World)
	{
		UE_LOG(RTFoliageInvoker, Warning,
			TEXT("URTMPCUpdater::PushVectorDataToMPC >> World is null — MPC not updated"));
		return;
	}

	const TArray<FRTPoolEntry>& Pool = PoolManager->GetPool();
	const int32 SlotCount = FMath::Min(Pool.Num(), ParamName_SlotData.Num());

	int32 ActiveCount = 0;

	for (int32 i = 0; i < SlotCount; ++i)
	{
		const FRTPoolEntry& Slot = Pool[i];
		const bool bActive = Slot.IsOccupied();

		const FLinearColor SlotData(
			bActive ? Slot.CellOriginWS.X : -9999999.f,
			bActive ? Slot.CellOriginWS.Y : -9999999.f,
			bActive ? 1.f : 0.f,
			static_cast<float>(i)
		);

		UKismetMaterialLibrary::SetVectorParameterValue(
			World, ParameterCollection, ParamName_SlotData[i], SlotData);

		if (bActive) { ++ActiveCount; }
	}

	// Zero out slots beyond the current pool size
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

	UE_LOG(RTFoliageInvoker, Log,
		TEXT("URTMPCUpdater::PushVectorDataToMPC >> CellSize=%.1f ActiveSlots=%d Slot0=(%.0f,%.0f,Z=%.1f)"),
		PoolManager->CellSize,
		ActiveCount,
		Pool.IsValidIndex(0) ? (Pool[0].IsOccupied() ? Pool[0].CellOriginWS.X : -9999999.f) : -1.f,
		Pool.IsValidIndex(0) ? (Pool[0].IsOccupied() ? Pool[0].CellOriginWS.Y : -9999999.f) : -1.f,
		Pool.IsValidIndex(0) ? (Pool[0].IsOccupied() ? 1.f : 0.f) : -1.f
	);
}

void URTMPCUpdater::PushTexturesForSlot(int32 SlotIndex)
{
	if (!PoolManager) { return; }
	if (!ParamName_ImpulseRT.IsValidIndex(SlotIndex)) { return; }

	const TArray<FRTPoolEntry>& Pool = PoolManager->GetPool();
	if (!Pool.IsValidIndex(SlotIndex)) { return; }

	const FRTPoolEntry& Slot = Pool[SlotIndex];

	for (UMaterialInstanceDynamic* MID : RegisteredMIDs)
	{
		if (!MID) { continue; }
		MID->SetTextureParameterValue(ParamName_ImpulseRT[SlotIndex],    Slot.ImpulseRT);
		MID->SetTextureParameterValue(ParamName_ContinuousRT[SlotIndex], Slot.ContinuousRT);
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
	PushTexturesForSlot(SlotIndex);

	UE_LOG(RTFoliageInvoker, Verbose,
		TEXT("URTMPCUpdater::OnCellAssigned >> Slot %d assigned to cell (%d,%d) — textures pushed to %d MIDs"),
		SlotIndex, CellIndex.X, CellIndex.Y, RegisteredMIDs.Num());
}

void URTMPCUpdater::OnCellReclaimed(FIntPoint CellIndex, int32 SlotIndex)
{
	UE_LOG(RTFoliageInvoker, Verbose,
		TEXT("URTMPCUpdater::OnCellReclaimed >> Slot %d reclaimed from cell (%d,%d)"),
		SlotIndex, CellIndex.X, CellIndex.Y);
}
