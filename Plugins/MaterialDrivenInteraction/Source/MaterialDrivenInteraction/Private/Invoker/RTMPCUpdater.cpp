#include "Invoker/RTMPCUpdater.h"

#include "Managers/RTPoolManager.h"
#include "Data/RTPoolTypes.h"
#include "Debug/LogCategory.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

const FName URTMPCUpdater::PN_CellSize        (TEXT("FRT_CellSize"));
const FName URTMPCUpdater::PN_ActiveSlotCount (TEXT("FRT_ActiveSlotCount"));

URTMPCUpdater::URTMPCUpdater()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup   = TG_PostUpdateWork;
}

void URTMPCUpdater::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetOwner() ? GetOwner()->GetWorld() : nullptr;
	if (!World)
	{
		UE_LOG(RTFoliageInvoker, Warning,
			TEXT("URTMPCUpdater::BeginPlay >> World is null — component disabled"));
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

	const FVector OwnerLoc = GetOwner()->GetActorLocation();
	UE_LOG(RTFoliageInvoker, Log,
		TEXT("URTMPCUpdater::BeginPlay >> Initialised — PoolSize=%d CellSize=%.0f MPC=%s  OwnerLoc(%.0f,%.0f,%.0f)"),
		PoolManager->PoolSize, PoolManager->CellSize,
		ParameterCollection ? *ParameterCollection->GetName() : TEXT("none"),
		OwnerLoc.X, OwnerLoc.Y, OwnerLoc.Z);
}

void URTMPCUpdater::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PoolManager)
	{
		PoolManager->OnCellAssigned.RemoveDynamic(this, &URTMPCUpdater::OnCellAssigned);
		PoolManager->OnCellReclaimed.RemoveDynamic(this, &URTMPCUpdater::OnCellReclaimed);
	}

	const FVector OwnerLoc = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
	UE_LOG(RTFoliageInvoker, Log,
		TEXT("URTMPCUpdater::EndPlay >> Delegates unbound  OwnerLoc(%.0f,%.0f,%.0f)"),
		OwnerLoc.X, OwnerLoc.Y, OwnerLoc.Z);

	Super::EndPlay(EndPlayReason);
}

void URTMPCUpdater::TickComponent(float DeltaTime, ELevelTick TickType,
                                   FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	PushVectorDataToMPC();

	/*if (PoolManager)
	{
		DebugPool = PoolManager->GetPool();
	}*/
}

UMaterialParameterCollectionInstance* URTMPCUpdater::GetMPCInstance() const
{
	if (!ParameterCollection || !GetOwner() || !GetOwner()->GetWorld()) { return nullptr; }
	return GetOwner()->GetWorld()->GetParameterCollectionInstance(ParameterCollection);
}

void URTMPCUpdater::BuildParameterNames()
{
	ParamName_SlotData.SetNum(FRT_MAX_POOL_SLOTS);
	ParamName_ImpulseRT.SetNum(FRT_MAX_POOL_SLOTS);
	ParamName_ContinuousRT.SetNum(FRT_MAX_POOL_SLOTS);

	for (int32 i = 0; i < FRT_MAX_POOL_SLOTS; ++i)
	{
		ParamName_SlotData[i]     = FName(*FString::Printf(TEXT("FRT_Slot%d_Data"),         i));
		ParamName_ImpulseRT[i]    = FName(*FString::Printf(TEXT("FRT_Slot%d_ImpulseRT"),    i));
		ParamName_ContinuousRT[i] = FName(*FString::Printf(TEXT("FRT_Slot%d_ContinuousRT"), i));
	}

	const int32 ActiveSlotCount = PoolManager
		? FMath::Min(PoolManager->PoolSize, FRT_MAX_POOL_SLOTS)
		: 0;

	UE_LOG(RTFoliageInvoker, Log,
		TEXT("URTMPCUpdater::BuildParameterNames >> Built %d entries (PoolSize=%d FRT_MAX_POOL_SLOTS=%d)"),
		FRT_MAX_POOL_SLOTS, ActiveSlotCount, FRT_MAX_POOL_SLOTS);
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
	const int32 SlotCount = FMath::Min(Pool.Num(), FRT_MAX_POOL_SLOTS);
	int32 ActiveCount = 0;

	for (int32 i = 0; i < SlotCount; ++i)
	{
		const FRTPoolEntry& Slot   = Pool[i];
		const bool          bActive = Slot.IsOccupied();

		UKismetMaterialLibrary::SetVectorParameterValue(
			World, ParameterCollection, ParamName_SlotData[i],
			FLinearColor(
				bActive ? Slot.CellOriginWS.X : -9999999.f,
				bActive ? Slot.CellOriginWS.Y : -9999999.f,
				bActive ? 1.f : 0.f,
				static_cast<float>(i)
			));

		if (bActive) { ++ActiveCount; }
	}

	for (int32 i = SlotCount; i < ParamName_SlotData.Num(); ++i)
	{
		UKismetMaterialLibrary::SetVectorParameterValue(
			World, ParameterCollection, ParamName_SlotData[i], FLinearColor::Black);
	}

	UKismetMaterialLibrary::SetScalarParameterValue(
		World, ParameterCollection, PN_CellSize, PoolManager->CellSize);

	UKismetMaterialLibrary::SetScalarParameterValue(
		World, ParameterCollection, PN_ActiveSlotCount, static_cast<float>(ActiveCount));

	const FVector OwnerLoc = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
	UE_LOG(RTFoliageInvoker, Verbose,
		TEXT("URTMPCUpdater::PushVectorDataToMPC >> CellSize=%.0f ActiveSlots=%d/%d Slot0=(%.0f,%.0f,Z=%.1f)  OwnerLoc(%.0f,%.0f,%.0f)"),
		PoolManager->CellSize, ActiveCount, SlotCount,
		Pool.IsValidIndex(0) ? (Pool[0].IsOccupied() ? Pool[0].CellOriginWS.X : -9999999.f) : -1.f,
		Pool.IsValidIndex(0) ? (Pool[0].IsOccupied() ? Pool[0].CellOriginWS.Y : -9999999.f) : -1.f,
		Pool.IsValidIndex(0) ? (Pool[0].IsOccupied() ? 1.f : 0.f) : -1.f,
		OwnerLoc.X, OwnerLoc.Y, OwnerLoc.Z);
}

void URTMPCUpdater::OnCellAssigned(FIntPoint CellIndex, int32 SlotIndex)
{
	const FVector OwnerLoc = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
	UE_LOG(RTFoliageInvoker, Verbose,
		TEXT("URTMPCUpdater::OnCellAssigned >> Slot %d → Cell (%d,%d)  OwnerLoc(%.0f,%.0f,%.0f)"),
		SlotIndex, CellIndex.X, CellIndex.Y,
		OwnerLoc.X, OwnerLoc.Y, OwnerLoc.Z);
}

void URTMPCUpdater::OnCellReclaimed(FIntPoint CellIndex, int32 SlotIndex)
{
	const FVector OwnerLoc = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
	UE_LOG(RTFoliageInvoker, Verbose,
		TEXT("URTMPCUpdater::OnCellReclaimed >> Slot %d reclaimed from Cell (%d,%d)  OwnerLoc(%.0f,%.0f,%.0f)"),
		SlotIndex, CellIndex.X, CellIndex.Y,
		OwnerLoc.X, OwnerLoc.Y, OwnerLoc.Z);
}