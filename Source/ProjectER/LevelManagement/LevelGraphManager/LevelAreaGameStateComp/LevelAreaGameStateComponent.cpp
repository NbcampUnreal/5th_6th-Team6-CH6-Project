#include "LevelAreaGameStateComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "LevelManagement/LevelAreaTrackerComponent.h"
#include "LevelManagement/Area/LevelAreaComponent.h"
#include "LevelManagement/LevelGraphManager/LevelAreaSubsystem/LevelAreaGraphSubsystem.h"
#include "LogHelper/DebugLogHelper.h"

ULevelAreaGameStateComponent::ULevelAreaGameStateComponent()
{
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = false;
}

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
    if (!World) return;

    ULevelAreaGraphSubsystem* Sub =
        World->GetSubsystem<ULevelAreaGraphSubsystem>();
    if (!Sub) return;

    CurrentPhase++;//increment phase

    // First phase: generate the full order for this round
    if (HazardOrder.IsEmpty())
    {
        int32 TotalNodes = Sub->Nodes.Num();
        // Leave at least one safe node — never hazard everything
        int32 MaxHazards = FMath::Max(0, TotalNodes - 1);

        if (!Sub->GenerateHazardOrder(MaxHazards, HazardOrder))
        {
            UE_LOG(LevelAreaGraphManagement, Warning,
                TEXT("AdvancePhase >> GenerateHazardOrder failed on phase %d"),
                CurrentPhase);
            return;
        }

        UE_LOG(LevelAreaGraphManagement, Log,
            TEXT("AdvancePhase >> Hazard order generated (%d nodes)"),
            HazardOrder.Num());
    }

    // Take the next batch from the pre-generated order
    int32 StartIdx = (CurrentPhase - 1) * HazardsPerPhase;
    if (StartIdx >= HazardOrder.Num())
    {
        UE_LOG(LevelAreaGraphManagement, Log,
            TEXT("AdvancePhase >> All hazards exhausted at phase %d"),
            CurrentPhase);
        return;
    }

    int32 EndIdx = FMath::Min(StartIdx + HazardsPerPhase, HazardOrder.Num());

    TArray<int32> NewHazards;
    for (int32 i = StartIdx; i < EndIdx; i++)
        NewHazards.Add(HazardOrder[i]);

    // Accumulate — phases are additive (previous hazards stay hazardous)
    for (int32 ID : NewHazards)
        HazardNodeIDs.AddUnique(ID);

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("AdvancePhase >> Phase %d | New hazards: %d | Total: %d"),
        CurrentPhase, NewHazards.Num(), HazardNodeIDs.Num());

    // Update server-side subsystem and room components
    Sub->ApplyHazardNodes(HazardNodeIDs);
    ApplyHazardToComponents(NewHazards, true);

    // Replication triggers OnRep_HazardNodeIDs on clients automatically
}

void ULevelAreaGameStateComponent::ResetHazards()
{
    HazardOrder.Reset();
    HazardNodeIDs.Reset();
    CurrentPhase = 0;

    ApplyHazardToComponents(HazardNodeIDs, false);

    if (ULevelAreaGraphSubsystem* Sub =
        GetWorld()->GetSubsystem<ULevelAreaGraphSubsystem>())
    {
        Sub->ClearHazards();
    }
}

/* =====================================================================
   Server: Notify Room Actors
   ===================================================================== */

void ULevelAreaGameStateComponent::ApplyHazardToComponents(
    const TArray<int32>& NodeIDs, bool bHazard)
{
    if (NodeIDs.IsEmpty()) return;

    UWorld* World = GetWorld();
    if (!World) return;

    // Iterate all actors with a LevelAreaComponent
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (ULevelAreaComponent* Comp =
            It->FindComponentByClass<ULevelAreaComponent>())
        {
            if (NodeIDs.Contains(Comp->NodeID))
            {
                Comp->SetHazard(bHazard);
            }
        }
    }
}

/* =====================================================================
   Client: OnRep
   ===================================================================== */

void ULevelAreaGameStateComponent::OnRep_HazardNodeIDs()
{
    // Mirror the replicated hazard set into the client's local subsystem
    NotifySubsystem(HazardNodeIDs);
}

void ULevelAreaGameStateComponent::NotifySubsystem(const TArray<int32>& NodeIDs)
{
    UWorld* World = GetWorld();
    if (!World) return;

    if (ULevelAreaGraphSubsystem* Sub =
        World->GetSubsystem<ULevelAreaGraphSubsystem>())
    {
        Sub->ApplyHazardNodes(NodeIDs);
    }

    // Tell any tracker sitting inside a newly-hazardous node to re-evaluate
    for (TActorIterator<APawn> It(World); It; ++It)
    {
        if (ULevelAreaTrackerComponent* Tracker =
            It->FindComponentByClass<ULevelAreaTrackerComponent>())
        {
            Tracker->RefreshHazardState();
        }
    }
}