#include "LevelAreaGameStateComponent.h"

#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"

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
   BeginPlay : Build graph + Generate hazard order
   ===================================================================== */

void ULevelAreaGameStateComponent::BeginPlay()
{
    Super::BeginPlay();

    if (!GetOwner()->HasAuthority())
        return;

    UWorld* World = GetWorld();
    if (!World)
        return;

    ULevelAreaGraphSubsystem* Sub =
        World->GetSubsystem<ULevelAreaGraphSubsystem>();

    if (!Sub)
        return;

    /* ---------- Build Graph From Data Asset ---------- */

    if (!GraphData)
    {
        UE_LOG(LevelAreaGraphManagement, Warning,
            TEXT("BeginPlay >> GraphData is NULL"));
        return;
    }

    for (const FLevelAreaNodeRecord& Record : GraphData->Nodes)
    {
        LevelAreaNode* Node = new LevelAreaNode();

        Node->SetNodeID(Record.NodeID);

        for (int32 NeighborID : Record.NeighborIDs)
        {
            Node->AddNeighbor(NeighborID);
        }

        Sub->RegisterNode(Node);
    }

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("BeginPlay >> Graph built (%d nodes)"),
        GraphData->Nodes.Num());

    /* ---------- Setup Seed ---------- */

    if (HazardSeed == 0)
    {
        HazardSeed = FMath::Rand();
    }

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("BeginPlay >> Hazard Seed: %d"),
        HazardSeed);
    
    /* ---------- Generate Hazard Order ---------- */

    int32 TotalNodes = Sub->Nodes.Num();
    int32 MaxHazards = FMath::Max(0, TotalNodes - 1);

    if (!Sub->GenerateHazardOrder(MaxHazards,HazardSeed, HazardOrder))
    {
        UE_LOG(LevelAreaGraphManagement, Warning,
            TEXT("BeginPlay >> Hazard order generation failed"));
        return;
    }

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("BeginPlay >> Hazard order generated (%d nodes)"),
        HazardOrder.Num());
}


/* =====================================================================
   Replication
   ===================================================================== */

void ULevelAreaGameStateComponent::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ULevelAreaGameStateComponent, HazardNodeIDs);
}


/* =====================================================================
   Server: Phase Advancement
   ===================================================================== */

void ULevelAreaGameStateComponent::AdvancePhase()
{
    UWorld* World = GetWorld();
    if (!World)
        return;

    ULevelAreaGraphSubsystem* Sub =
        World->GetSubsystem<ULevelAreaGraphSubsystem>();

    if (!Sub)
        return;

    if (HazardOrder.IsEmpty())
    {
        UE_LOG(LevelAreaGraphManagement, Warning,
            TEXT("AdvancePhase >> HazardOrder empty (generation likely failed)"));
        return;
    }

    CurrentPhase++;

    int32 StartIdx = (CurrentPhase - 1) * HazardsPerPhase;

    if (StartIdx >= HazardOrder.Num())
    {
        UE_LOG(LevelAreaGraphManagement, Log,
            TEXT("AdvancePhase >> All hazards exhausted at phase %d"),
            CurrentPhase);
        return;
    }

    int32 EndIdx =
        FMath::Min(StartIdx + HazardsPerPhase, HazardOrder.Num());

    TArray<int32> NewHazards;

    for (int32 i = StartIdx; i < EndIdx; i++)
    {
        NewHazards.Add(HazardOrder[i]);
    }

    for (int32 ID : NewHazards)
    {
        HazardNodeIDs.AddUnique(ID);
    }

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("AdvancePhase >> Phase %d | New: %d | Total: %d | State: %s"),
        CurrentPhase,
        NewHazards.Num(),
        HazardNodeIDs.Num(),
        *UEnum::GetValueAsString(HazardStatePerPhase));

    Sub->ApplyHazardNodes(NewHazards, HazardStatePerPhase);
}


/* =====================================================================
   Reset
   ===================================================================== */

void ULevelAreaGameStateComponent::ResetHazards()
{
    HazardOrder.Reset();
    HazardNodeIDs.Reset();
    CurrentPhase = 0;

    if (ULevelAreaGraphSubsystem* Sub =
        GetWorld()->GetSubsystem<ULevelAreaGraphSubsystem>())
    {
        Sub->ClearHazards();
    }

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("ResetHazards >> Round reset complete"));
}


/* =====================================================================
   Client: Replication
   ===================================================================== */

void ULevelAreaGameStateComponent::OnRep_HazardNodeIDs()
{
    NotifySubsystem(HazardNodeIDs);
}


void ULevelAreaGameStateComponent::NotifySubsystem(
    const TArray<int32>& NodeIDs)
{
    UWorld* World = GetWorld();
    if (!World)
        return;

    if (ULevelAreaGraphSubsystem* Sub =
        World->GetSubsystem<ULevelAreaGraphSubsystem>())
    {
        Sub->ApplyHazardNodes(NodeIDs, HazardStatePerPhase);
    }

    for (TActorIterator<APawn> It(World); It; ++It)
    {
        if (ULevelAreaTrackerComponent* Tracker =
            It->FindComponentByClass<ULevelAreaTrackerComponent>())
        {
            Tracker->RefreshHazardState();
        }
    }
}