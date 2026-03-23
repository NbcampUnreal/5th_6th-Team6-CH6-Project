#include "LevelAreaGameStateComponent.h"

#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "EngineUtils.h"

#include "LevelManagement/Requirements/LevelAreaGraphData.h"
#include "LevelManagement/LevelAreaTrackerComponent.h"
#include "LevelManagement/LevelGraphManager/LevelAreaSubsystem/LevelAreaGraphSubsystem.h"
#include "LevelManagement/Area/LevelAreaInstanceBridge.h"

#include "LogHelper/DebugLogHelper.h"


ULevelAreaGameStateComponent::ULevelAreaGameStateComponent()
{
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = false;
}


/* =====================================================================
   BeginPlay — build graph + generate full hazard order once
   ===================================================================== */

void ULevelAreaGameStateComponent::BeginPlay()
{
    Super::BeginPlay();

    if (!GetOwner()->HasAuthority())
        return;

    UWorld* World = GetWorld();
    if (!World) return;

    ULevelAreaGraphSubsystem* Sub =
        World->GetSubsystem<ULevelAreaGraphSubsystem>();
    if (!Sub) return;

    /* ---------- Build Graph ---------- */

    if (!GraphData)
    {
        UE_LOG(LevelAreaGraphManagement, Warning,
            TEXT("BeginPlay >> GraphData is null"));
        return;
    }

    for (const FLevelAreaNodeRecord& Record : GraphData->Nodes)
    {
        LevelAreaNode* Node = new LevelAreaNode();
        Node->SetNodeID(Record.NodeID);

        for (int32 NeighborID : Record.NeighborIDs)
            Node->AddNeighbor(NeighborID);

        Sub->RegisterNode(Node);
    }

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("BeginPlay >> Graph built (%d nodes)"), GraphData->Nodes.Num());

    /*/* ---------- Build Bridge Actor Map ---------- #1#

    BuildBridgeActorMap();*/

    /* ---------- Seed ---------- */

    if (HazardSeed == 0)
        HazardSeed = FMath::Rand();

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("BeginPlay >> Seed: %d"), HazardSeed);

    /* ---------- Generate Full Collapse Order ---------- */

    int32 MaxHazards = FMath::Max(0, Sub->Nodes.Num() - 1);

    if (!Sub->GenerateHazardOrder(MaxHazards, HazardSeed, HazardOrder))
    {
        UE_LOG(LevelAreaGraphManagement, Warning,
            TEXT("BeginPlay >> Hazard order generation failed"));
        return;
    }

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("BeginPlay >> Hazard order ready (%d entries)"), HazardOrder.Num());
}


/* =====================================================================
   Replication
   ===================================================================== */

void ULevelAreaGameStateComponent::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ULevelAreaGameStateComponent, HazardOrder);
    DOREPLIFETIME(ULevelAreaGameStateComponent, CurrentPhase);
}

void ULevelAreaGameStateComponent::RegisterBridge(ALevelAreaInstanceBridge* Bridge)
{
    if (!Bridge || Bridge->NodeID == INDEX_NONE) return;

    BridgeActorMap.FindOrAdd(Bridge->NodeID).AddUnique(Bridge);

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("RegisterBridge >> NodeID %d → %s"),
        Bridge->NodeID, *Bridge->GetName());
}

void ULevelAreaGameStateComponent::UnregisterBridge(ALevelAreaInstanceBridge* Bridge)
{
    if (!Bridge || Bridge->NodeID == INDEX_NONE) return;

    if (TArray<TObjectPtr<ALevelAreaInstanceBridge>>* Found =
        BridgeActorMap.Find(Bridge->NodeID))
    {
        Found->Remove(Bridge);

        if (Found->IsEmpty())
            BridgeActorMap.Remove(Bridge->NodeID);
    }

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("UnregisterBridge >> NodeID %d → %s"),
        Bridge->NodeID, *Bridge->GetName());
}


/* =====================================================================
   Server: Phase Advancement
   ===================================================================== */

void ULevelAreaGameStateComponent::AdvancePhase()
{
    if (HazardOrder.IsEmpty())
    {
        UE_LOG(LevelAreaGraphManagement, Warning,
            TEXT("AdvancePhase >> HazardOrder is empty"));
        return;
    }

    int32 StartIdx = CurrentPhase * HazardsPerPhase;

    if (StartIdx >= HazardOrder.Num())
    {
        UE_LOG(LevelAreaGraphManagement, Log,
            TEXT("AdvancePhase >> All phases exhausted at phase %d"), CurrentPhase);
        return;
    }

    CurrentPhase++;

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("AdvancePhase >> Phase %d | State: %s"),
        CurrentPhase, *UEnum::GetValueAsString(HazardStatePerPhase));

    ApplyHazardsUpToCurrentPhase();
}


/* =====================================================================
   Reset
   ===================================================================== */

void ULevelAreaGameStateComponent::ResetHazards()
{
    CurrentPhase = 0;

    if (ULevelAreaGraphSubsystem* Sub =
        GetWorld()->GetSubsystem<ULevelAreaGraphSubsystem>())
    {
        Sub->ClearHazards();
    }

    NotifyBridgeActors({});
    NotifyTrackers();

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("ResetHazards >> Reset complete"));
}


/* =====================================================================
   OnRep
   ===================================================================== */

void ULevelAreaGameStateComponent::OnRep_HazardOrder()
{
    if (CurrentPhase > 0)
        ApplyHazardsUpToCurrentPhase();

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("OnRep_HazardOrder >> Received %d entries"), HazardOrder.Num());
}

void ULevelAreaGameStateComponent::OnRep_CurrentPhase()
{
    ApplyHazardsUpToCurrentPhase();

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("OnRep_CurrentPhase >> Phase %d received"), CurrentPhase);
}


/* =====================================================================
   Internal
   ===================================================================== */

void ULevelAreaGameStateComponent::ApplyHazardsUpToCurrentPhase()
{
    if (HazardOrder.IsEmpty()) return;

    UWorld* World = GetWorld();
    if (!World) return;

    ULevelAreaGraphSubsystem* Sub =
        World->GetSubsystem<ULevelAreaGraphSubsystem>();
    if (!Sub) return;

    int32 ActiveCount = FMath::Min(CurrentPhase * HazardsPerPhase, HazardOrder.Num());

    TArray<int32> ActiveHazards;
    for (int32 i = 0; i < ActiveCount; i++)
        ActiveHazards.Add(HazardOrder[i]);

    Sub->ClearHazards();
    Sub->ApplyHazardNodes(ActiveHazards, HazardStatePerPhase);

    NotifyBridgeActors(ActiveHazards);
    NotifyTrackers();

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("ApplyHazardsUpToCurrentPhase >> Applied %d hazards for phase %d"),
        ActiveHazards.Num(), CurrentPhase);
}

void ULevelAreaGameStateComponent::BuildBridgeActorMap()
{
    BridgeActorMap.Reset();

    UWorld* World = GetWorld();
    if (!World) return;

    for (ULevel* Level : World->GetLevels())
    {
        if (!Level) continue;

        for (AActor* Actor : Level->Actors)
        {
            ALevelAreaInstanceBridge* Bridge = Cast<ALevelAreaInstanceBridge>(Actor);

            if (!Bridge || Bridge->NodeID == INDEX_NONE)
                continue;

            BridgeActorMap.FindOrAdd(Bridge->NodeID).Add(Bridge);

            UE_LOG(LevelAreaGraphManagement, Log,
                TEXT("BuildBridgeActorMap >> NodeID %d → %s"),
                Bridge->NodeID, *Bridge->GetName());
        }
    }

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("BuildBridgeActorMap >> Total: %d node entries"), BridgeActorMap.Num());
}

TArray<ALevelAreaInstanceBridge*> ULevelAreaGameStateComponent::GetBridgeActorsByID(int32 NodeID) const
{
    TArray<ALevelAreaInstanceBridge*> Result;

    const TArray<TObjectPtr<ALevelAreaInstanceBridge>>* Found = BridgeActorMap.Find(NodeID);

    if (!Found || Found->IsEmpty())
    {
        UE_LOG(LevelAreaGraphManagement, Warning,
            TEXT("GetBridgeActorsByID >> NodeID %d not found"), NodeID);
        return Result;
    }

    for (const TObjectPtr<ALevelAreaInstanceBridge>& Ptr : *Found)
    {
        if (Ptr)
            Result.Add(Ptr.Get());
    }

    return Result;
}

ALevelAreaInstanceBridge* ULevelAreaGameStateComponent::GetBridgeActorByInstance(
    ALevelInstance* LevelInstance) const
{
    for (const TPair<int32, TArray<TObjectPtr<ALevelAreaInstanceBridge>>>& Pair : BridgeActorMap)
    {
        for (const TObjectPtr<ALevelAreaInstanceBridge>& Bridge : Pair.Value)
        {
            if (Bridge && Bridge->GetOwningLevelInstance() == LevelInstance)
                return Bridge.Get();
        }
    }

    UE_LOG(LevelAreaGraphManagement, Warning,
        TEXT("GetBridgeActorByInstance >> No bridge found for given LevelInstance"));

    return nullptr;
}

void ULevelAreaGameStateComponent::NotifyBridgeActors(const TArray<int32>& HazardNodeIDs)
{
    for (const TPair<int32, TArray<TObjectPtr<ALevelAreaInstanceBridge>>>& Pair : BridgeActorMap)
    {
        EAreaHazardState State = HazardNodeIDs.Contains(Pair.Key)
            ? HazardStatePerPhase
            : EAreaHazardState::Safe;

        for (const TObjectPtr<ALevelAreaInstanceBridge>& Bridge : Pair.Value)
        {
            if (Bridge)
                Bridge->NotifyHazardStateChanged(State);
        }
    }
}

void ULevelAreaGameStateComponent::NotifyTrackers()
{
    UWorld* World = GetWorld();
    if (!World) return;

    for (TActorIterator<APawn> It(World); It; ++It)
    {
        if (ULevelAreaTrackerComponent* Tracker =
            It->FindComponentByClass<ULevelAreaTrackerComponent>())
        {
            Tracker->RefreshHazardState();
        }
    }
}