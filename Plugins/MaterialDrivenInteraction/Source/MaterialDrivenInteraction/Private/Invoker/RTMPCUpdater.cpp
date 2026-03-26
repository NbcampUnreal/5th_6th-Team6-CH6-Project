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
const FName URTMPCUpdater::PN_NowFrac         (TEXT("FRT_NowFrac"));
const FName URTMPCUpdater::PN_Period          (TEXT("FRT_Period"));

URTMPCUpdater::URTMPCUpdater()
{
    PrimaryComponentTick.bCanEverTick          = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.TickGroup             = TG_PostUpdateWork;
}

void URTMPCUpdater::BeginPlay()
{
    Super::BeginPlay();
    // Intentionally empty — wait for InitializeMPCUpdater from local player
}

void URTMPCUpdater::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    DeinitializeMPCUpdater();

    UE_LOG(RTFoliageInvoker, Log,
        TEXT("URTMPCUpdater::EndPlay >> Loc(%.0f,%.0f,%.0f)"),
        GetSourceLocation().X, GetSourceLocation().Y, GetSourceLocation().Z);

    Super::EndPlay(EndPlayReason);
}

void URTMPCUpdater::TickComponent(float DeltaTime, ELevelTick TickType,
                                   FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (PoolManager)
    {
        PoolManager->SetPriorityCenter(GetSourceLocation());
        PoolManager->SetDrawRadius(DrawRadius);
    }

    PushVectorDataToMPC();
}

bool URTMPCUpdater::InitializeMPCUpdater(USceneComponent* InRootComponent)
{
    if (bInitialized) { return false; }

    if (!InRootComponent)
    {
        UE_LOG(RTFoliageInvoker, Warning,
            TEXT("URTMPCUpdater::InitializeMPCUpdater >> InRootComponent is null"));
        return false;
    }

    UWorld* World = GetOwner() ? GetOwner()->GetWorld() : nullptr;
    if (!World)
    {
        UE_LOG(RTFoliageInvoker, Warning,
            TEXT("URTMPCUpdater::InitializeMPCUpdater >> World is null"));
        return false;
    }

    PoolManager = World->GetSubsystem<URTPoolManager>();
    if (!PoolManager)
    {
        UE_LOG(RTFoliageInvoker, Warning,
            TEXT("URTMPCUpdater::InitializeMPCUpdater >> URTPoolManager not found"));
        return false;
    }

    CachedRootComponent = InRootComponent;

    PoolManager->OnCellAssigned.AddDynamic(this, &URTMPCUpdater::OnCellAssigned);
    PoolManager->OnCellReclaimed.AddDynamic(this, &URTMPCUpdater::OnCellReclaimed);

    BuildParameterNames();
    PushVectorDataToMPC();

    SetComponentTickEnabled(true);
    bInitialized = true;

    UE_LOG(RTFoliageInvoker, Log,
        TEXT("URTMPCUpdater::InitializeMPCUpdater >> Activated — PoolSize=%d CellSize=%.0f Period=%.1f DrawRadius=%.0f MPC=%s  Loc(%.0f,%.0f,%.0f)"),
        PoolManager->PoolSize, PoolManager->CellSize, TimestampPeriod, DrawRadius,
        ParameterCollection ? *ParameterCollection->GetName() : TEXT("none"),
        GetSourceLocation().X, GetSourceLocation().Y, GetSourceLocation().Z);

    return true;
}

void URTMPCUpdater::DeinitializeMPCUpdater()
{
    if (!bInitialized) { return; }

    SetComponentTickEnabled(false);
    CachedRootComponent = nullptr;

    if (PoolManager)
    {
        PoolManager->OnCellAssigned.RemoveDynamic(this, &URTMPCUpdater::OnCellAssigned);
        PoolManager->OnCellReclaimed.RemoveDynamic(this, &URTMPCUpdater::OnCellReclaimed);
    }

    bInitialized = false;

    UE_LOG(RTFoliageInvoker, Log,
        TEXT("URTMPCUpdater::DeinitializeMPCUpdater >> Deactivated on %s"),
        GetOwner() ? *GetOwner()->GetName() : TEXT("null"));
}

UMaterialParameterCollectionInstance* URTMPCUpdater::GetMPCInstance() const
{
    if (!ParameterCollection || !GetOwner() || !GetOwner()->GetWorld()) { return nullptr; }
    return GetOwner()->GetWorld()->GetParameterCollectionInstance(ParameterCollection);
}

FVector URTMPCUpdater::GetSourceLocation() const
{
    if (CachedRootComponent)
        return CachedRootComponent->GetComponentLocation();

    if (GetOwner())
        return GetOwner()->GetActorLocation();

    return FVector::ZeroVector;
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

    UE_LOG(RTFoliageInvoker, Log,
        TEXT("URTMPCUpdater::BuildParameterNames >> Built %d entries (PoolSize=%d FRT_MAX_POOL_SLOTS=%d)"),
        FRT_MAX_POOL_SLOTS,
        PoolManager ? FMath::Min(PoolManager->PoolSize, FRT_MAX_POOL_SLOTS) : 0,
        FRT_MAX_POOL_SLOTS);
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
        const FRTPoolEntry& Slot    = Pool[i];
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

    const float SafePeriod = FMath::Max(TimestampPeriod, 1.f);
    const float NowFrac    = FMath::Frac(World->GetTimeSeconds() / SafePeriod);

    UKismetMaterialLibrary::SetScalarParameterValue(
        World, ParameterCollection, PN_NowFrac, NowFrac);

    UKismetMaterialLibrary::SetScalarParameterValue(
        World, ParameterCollection, PN_Period, SafePeriod);

    const FVector Loc = GetSourceLocation();
    UE_LOG(RTFoliageInvoker, Verbose,
        TEXT("URTMPCUpdater::PushVectorDataToMPC >> CellSize=%.0f ActiveSlots=%d/%d NowFrac=%.4f Period=%.1f DrawRadius=%.0f  Loc(%.0f,%.0f,%.0f)"),
        PoolManager->CellSize, ActiveCount, SlotCount, NowFrac, SafePeriod, DrawRadius,
        Loc.X, Loc.Y, Loc.Z);
}

void URTMPCUpdater::OnCellAssigned(FIntPoint CellIndex, int32 SlotIndex)
{
    const FVector Loc = GetSourceLocation();
    UE_LOG(RTFoliageInvoker, Verbose,
        TEXT("URTMPCUpdater::OnCellAssigned >> Slot %d → Cell (%d,%d)  Loc(%.0f,%.0f,%.0f)"),
        SlotIndex, CellIndex.X, CellIndex.Y, Loc.X, Loc.Y, Loc.Z);
}

void URTMPCUpdater::OnCellReclaimed(FIntPoint CellIndex, int32 SlotIndex)
{
    const FVector Loc = GetSourceLocation();
    UE_LOG(RTFoliageInvoker, Verbose,
        TEXT("URTMPCUpdater::OnCellReclaimed >> Slot %d reclaimed from Cell (%d,%d)  Loc(%.0f,%.0f,%.0f)"),
        SlotIndex, CellIndex.X, CellIndex.Y, Loc.X, Loc.Y, Loc.Z);
}