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

    GenerateGraph();
}

void ULevelAreaGameStateComponent::GenerateGraph()
{
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

    /* ---------- Build Bridge Actor Map ---------- */

    /*BuildBridgeActorMap();*/  // temp no level sequence play map yet

    /* ---------- Seed ---------- */

    if (!bUseFixedSeed)// use flag instead, not just 0
    {
        HazardSeed = FMath::Rand();  //rand seed
    }

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

void ULevelAreaGameStateComponent::AdvancePhase(int32 Phase)
{
    /*if (HazardOrder.IsEmpty())
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

    ApplyHazardsUpToCurrentPhase();*/

    if (HazardOrder.IsEmpty())
    {
        UE_LOG(LevelAreaGraphManagement, Warning,
            TEXT("AdvancePhase >> HazardOrder is empty"));
        return;
    }

    if (CurrentPhase * HazardsPerPhase >= HazardOrder.Num())
    {
        UE_LOG(LevelAreaGraphManagement, Log,
            TEXT("AdvancePhase >> All phases exhausted at phase %d"), CurrentPhase);
        return;
    }

    CurrentPhase = Phase;

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("AdvancePhase >> Phase %d | State: %s"),
        CurrentPhase, *UEnum::GetValueAsString(HazardStatePerPhase));

    ApplyHazards(CurrentPhase, HazardStatePerPhase);
}


/* =====================================================================
   Reset
   ===================================================================== */

void ULevelAreaGameStateComponent::ResetHazards(EAreaHazardState NewState)
{
    CancelAllInstantDeath();

    CurrentPhase = 0;

    if (ULevelAreaGraphSubsystem* Sub =
        GetWorld()->GetSubsystem<ULevelAreaGraphSubsystem>())
    {
        Sub->ClearHazards();
    }

    // Reset all bridges to None
    for (const TPair<int32, TArray<TObjectPtr<ALevelAreaInstanceBridge>>>& Pair : BridgeActorMap)
    {
        NotifyBridgeActors({ Pair.Key }, NewState);
    }

    NotifyTrackers();

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("ResetHazards >> Reset complete"));
}

void ULevelAreaGameStateComponent::ScheduleInstantDeath(int32 NodeID, float Delay)
{
    UWorld* World = GetWorld();
    if (!World) return;

    // Cancel any existing timer for this node before scheduling a new one
    CancelInstantDeath(NodeID);

    FTimerHandle Handle;
    FTimerDelegate Delegate;
    Delegate.BindUObject(this, &ULevelAreaGameStateComponent::ApplyInstantDeathToNode, NodeID);

    World->GetTimerManager().SetTimer(Handle, Delegate, Delay, false);
    InstantDeathTimerMap.Add(NodeID, Handle);

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("ScheduleInstantDeath >> NodeID %d will escalate in %.2fs"), NodeID, Delay);
}

void ULevelAreaGameStateComponent::CancelInstantDeath(int32 NodeID)
{
    UWorld* World = GetWorld();
    if (!World) return;

    if (FTimerHandle* Handle = InstantDeathTimerMap.Find(NodeID))
    {
        World->GetTimerManager().ClearTimer(*Handle);
        InstantDeathTimerMap.Remove(NodeID);

        UE_LOG(LevelAreaGraphManagement, Log,
            TEXT("CancelInstantDeath >> NodeID %d escalation cancelled"), NodeID);
    }
}

void ULevelAreaGameStateComponent::ScheduleInstantDeathForAllHazards(float Delay)
{
    UWorld* World = GetWorld();
    if (!World) return;

    ULevelAreaGraphSubsystem* Sub = World->GetSubsystem<ULevelAreaGraphSubsystem>();
    if (!Sub) return;

    for (const TPair<int32, EAreaHazardState>& Pair : Sub->ActiveHazardNodes)
    {
        // Only escalate nodes that are currently Hazard — skip already InstantDeath nodes
        if (Pair.Value == EAreaHazardState::Hazard)
            ScheduleInstantDeath(Pair.Key, Delay);
    }

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("ScheduleInstantDeathForAllHazards >> %d nodes scheduled (%.2fs delay)"),
        InstantDeathTimerMap.Num(), Delay);
}

void ULevelAreaGameStateComponent::CancelAllInstantDeath()
{
    UWorld* World = GetWorld();
    if (!World) return;

    for (TPair<int32, FTimerHandle>& Pair : InstantDeathTimerMap)
        World->GetTimerManager().ClearTimer(Pair.Value);

    InstantDeathTimerMap.Reset();

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("CancelAllInstantDeath >> All escalations cancelled"));
}

void ULevelAreaGameStateComponent::ApplyHazards(int32 Phase, EAreaHazardState State)
{
    if (HazardOrder.IsEmpty()) return;

    UWorld* World = GetWorld();
    if (!World) return;

    ULevelAreaGraphSubsystem* Sub =
        World->GetSubsystem<ULevelAreaGraphSubsystem>();
    if (!Sub) return;

    // Full cumulative range from the beginning up to this phase
    int32 PrevCount = FMath::Min((Phase - 1) * HazardsPerPhase, HazardOrder.Num());
    int32 ActiveCount = FMath::Min(Phase * HazardsPerPhase, HazardOrder.Num());

    // All nodes that should be active hazards at this phase
    TArray<int32> ActiveHazards;
    for (int32 i = 0; i < ActiveCount; i++)
        ActiveHazards.Add(HazardOrder[i]);

    // Only the nodes that are NEW this phase — bridges for already-hazardous
    // nodes must NOT be re-notified or they restart their transition sequences
    TArray<int32> NewHazards;
    for (int32 i = PrevCount; i < ActiveCount; i++)
        NewHazards.Add(HazardOrder[i]);

    // Rebuild subsystem state from scratch each time for correctness
    Sub->ClearHazards();
    Sub->ApplyHazardNodes(ActiveHazards, State);

    // Only notify bridges for newly hazardous nodes this phase
    NotifyBridgeActors(NewHazards, State);
    NotifyTrackers();

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("ApplyHazards >> Phase %d | New: %d | Total Active: %d | State: %s"),
        Phase, NewHazards.Num(), ActiveHazards.Num(), *UEnum::GetValueAsString(State));
}

void ULevelAreaGameStateComponent::NotifyBridgeActors(const TArray<int32>& NodeIDs, EAreaHazardState State)
{
    for (int32 NodeID : NodeIDs)
    {
        if (TArray<TObjectPtr<ALevelAreaInstanceBridge>>* Bridges =
            BridgeActorMap.Find(NodeID))
        {
            for (const TObjectPtr<ALevelAreaInstanceBridge>& Bridge : *Bridges)
            {
                if (Bridge)
                    Bridge->NotifyHazardStateChanged(State);
            }
        }
    }
}

void ULevelAreaGameStateComponent::ApplyInstantDeathToNode(int32 NodeID)
{
    // Remove the timer entry — it has already fired
    InstantDeathTimerMap.Remove(NodeID);

    UWorld* World = GetWorld();
    if (!World) return;

    ULevelAreaGraphSubsystem* Sub = World->GetSubsystem<ULevelAreaGraphSubsystem>();
    if (!Sub) return;

    // Safety check — node may have been reset or removed before the timer fired
    if (!Sub->IsNodeHazard(NodeID))
    {
        UE_LOG(LevelAreaGraphManagement, Warning,
            TEXT("ApplyInstantDeathToNode >> NodeID %d is no longer a hazard, skipping"), NodeID);
        return;
    }

    // Directly overwrite the subsystem state for this node only
    Sub->ActiveHazardNodes.Add(NodeID, EAreaHazardState::InstantDeath);

    NotifyBridgeActors({ NodeID }, EAreaHazardState::InstantDeath);
    NotifyTrackers();

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("ApplyInstantDeathToNode >> NodeID %d escalated to InstantDeath"), NodeID);
}


/* =====================================================================
   OnRep
   ===================================================================== */

void ULevelAreaGameStateComponent::OnRep_HazardOrder()
{
    // HazardOrder just arrived on the client — if phase is already ahead
    // (i.e. CurrentPhase replicated first), apply immediately to catch up
    if (CurrentPhase > 0)
        ApplyHazards(CurrentPhase, HazardStatePerPhase);

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("OnRep_HazardOrder >> Received %d entries"), HazardOrder.Num());
}

void ULevelAreaGameStateComponent::OnRep_CurrentPhase()
{
    // Guard: if HazardOrder hasn't arrived yet, wait for OnRep_HazardOrder to apply
    if (HazardOrder.IsEmpty()) return;

    ApplyHazards(CurrentPhase, HazardStatePerPhase);

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("OnRep_CurrentPhase >> Phase %d received"), CurrentPhase);
}


/* =====================================================================
   Internal
   ===================================================================== */



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