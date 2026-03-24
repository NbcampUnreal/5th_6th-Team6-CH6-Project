#include "LevelSequenceReplicatorComponent.h"

#include "LevelSequenceManagerSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"

ULevelSequenceReplicatorComponent::ULevelSequenceReplicatorComponent()
{
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = false;
}

void ULevelSequenceReplicatorComponent::Multicast_PlaySequence_Implementation(FName SequenceName)
{
    // Stamp start time and play from beginning
    float ServerTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
    SequenceStartTimes.Add(SequenceName, ServerTime);

    if (ULevelSequenceManagerSubsystem* Sub =
        GetWorld()->GetSubsystem<ULevelSequenceManagerSubsystem>())
    {
        Sub->PlaySequenceAtTime_Local(SequenceName, ServerTime);
    }
}

void ULevelSequenceReplicatorComponent::Multicast_StopSequence_Implementation(FName SequenceName)
{
    SequenceStartTimes.Remove(SequenceName);

    if (ULevelSequenceManagerSubsystem* Sub =
        GetWorld()->GetSubsystem<ULevelSequenceManagerSubsystem>())
    {
        Sub->StopSequence_Local(SequenceName);
    }
}

void ULevelSequenceReplicatorComponent::Multicast_PauseSequence_Implementation(FName SequenceName)
{
    if (ULevelSequenceManagerSubsystem* Sub =
        GetWorld()->GetSubsystem<ULevelSequenceManagerSubsystem>())
    {
        Sub->PauseSequence_Local(SequenceName);
    }
}

void ULevelSequenceReplicatorComponent::Multicast_ResetSequence_Implementation(FName SequenceName)
{
    SequenceStartTimes.Remove(SequenceName);

    if (ULevelSequenceManagerSubsystem* Sub =
        GetWorld()->GetSubsystem<ULevelSequenceManagerSubsystem>())
    {
        Sub->ResetSequence_Local(SequenceName);
    }
}

void ULevelSequenceReplicatorComponent::Multicast_PlaySequenceAtTime_Implementation(
    FName SequenceName, float ServerStartTime)
{
    SequenceStartTimes.Add(SequenceName, ServerStartTime);

    if (ULevelSequenceManagerSubsystem* Sub =
        GetWorld()->GetSubsystem<ULevelSequenceManagerSubsystem>())
    {
        Sub->PlaySequenceAtTime_Local(SequenceName, ServerStartTime);
    }
}

float ULevelSequenceReplicatorComponent::GetSequenceLoopPosition(FName SequenceName) const
{
    const float* StartTime = SequenceStartTimes.Find(SequenceName);
    if (!StartTime) return 0.f;

    float CurrentServerTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
    return CurrentServerTime - *StartTime;
}

bool ULevelSequenceReplicatorComponent::IsSequencePlaying(FName SequenceName) const
{
    return SequenceStartTimes.Contains(SequenceName);
}