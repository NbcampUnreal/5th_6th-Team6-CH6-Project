#include "LevelAreaGameStateComponent.h"

#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "EngineUtils.h"

#include "LevelManagement/Requirements/LevelAreaGraphData.h"
#include "LevelManagement/LevelAreaTrackerComponent.h"
#include "LevelManagement/LevelGraphManager/LevelAreaSubsystem/LevelAreaGraphSubsystem.h"

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

    // HazardOrder is now replicated to clients via OnRep_HazardOrder
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

    // Increment phase — replicates to clients, triggering OnRep_CurrentPhase
    CurrentPhase++;

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("AdvancePhase >> Phase %d | State: %s"),
        CurrentPhase, *UEnum::GetValueAsString(HazardStatePerPhase));

    // Apply on server directly
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

    NotifyTrackers();

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("ResetHazards >> Reset complete"));
}


/* =====================================================================
   OnRep
   ===================================================================== */

void ULevelAreaGameStateComponent::OnRep_HazardOrder()
{
    // HazardOrder just arrived on client — if phase is already set, apply immediately
    // (handles late joiners where CurrentPhase replicated before HazardOrder)
    if (CurrentPhase > 0)
        ApplyHazardsUpToCurrentPhase();

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("OnRep_HazardOrder >> Received %d entries"), HazardOrder.Num());
}

void ULevelAreaGameStateComponent::OnRep_CurrentPhase()
{
    // CurrentPhase updated — apply all hazards up to this phase
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

    // Derive the full active hazard set from HazardOrder up to CurrentPhase
    int32 ActiveCount = FMath::Min(CurrentPhase * HazardsPerPhase, HazardOrder.Num());

    TArray<int32> ActiveHazards;
    for (int32 i = 0; i < ActiveCount; i++)
        ActiveHazards.Add(HazardOrder[i]);

    Sub->ClearHazards();
    Sub->ApplyHazardNodes(ActiveHazards, HazardStatePerPhase);

    NotifyTrackers();

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("ApplyHazardsUpToCurrentPhase >> Applied %d hazards for phase %d"),
        ActiveHazards.Num(), CurrentPhase);
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