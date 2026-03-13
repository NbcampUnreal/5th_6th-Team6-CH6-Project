#include "LevelAreaGameStateComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "LevelManagement/LevelAreaTrackerComponent.h"
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

    CurrentPhase++;

    // Generate the full order once at the start of the round
    if (HazardOrder.IsEmpty())
    {
        int32 TotalNodes = Sub->Nodes.Num();

        // Always leave at least one safe node
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

    // Accumulate — previous hazards remain
    for (int32 ID : NewHazards)
        HazardNodeIDs.AddUnique(ID);

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("AdvancePhase >> Phase %d | New: %d | Total: %d | State: %s"),
        CurrentPhase, NewHazards.Num(), HazardNodeIDs.Num(),
        *UEnum::GetValueAsString(HazardStatePerPhase));

    // Apply to server-side subsystem — clients get it via OnRep
    Sub->ApplyHazardNodes(NewHazards, HazardStatePerPhase);
}

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
   Client: OnRep
   ===================================================================== */

void ULevelAreaGameStateComponent::OnRep_HazardNodeIDs()
{
    NotifySubsystem(HazardNodeIDs);
}

void ULevelAreaGameStateComponent::NotifySubsystem(const TArray<int32>& NodeIDs)
{
    UWorld* World = GetWorld();
    if (!World) return;

    if (ULevelAreaGraphSubsystem* Sub =
        World->GetSubsystem<ULevelAreaGraphSubsystem>())
    {
        Sub->ApplyHazardNodes(NodeIDs, HazardStatePerPhase);
    }

    // Push refresh to any tracker already standing in a newly hazardous node
    for (TActorIterator<APawn> It(World); It; ++It)
    {
        if (ULevelAreaTrackerComponent* Tracker =
            It->FindComponentByClass<ULevelAreaTrackerComponent>())
        {
            Tracker->RefreshHazardState();
        }
    }
}