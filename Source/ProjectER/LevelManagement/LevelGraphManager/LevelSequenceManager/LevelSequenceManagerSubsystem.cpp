#include "LevelSequenceManagerSubsystem.h"

#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "LevelSequenceReplicatorComponent.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "LevelManagement/Area/LevelAreaInstanceBridge.h"
#include "LevelManagement/LevelGraphManager/LevelAreaGameStateComp/LevelAreaGameStateComponent.h"
#include "LogHelper/DebugLogHelper.h"


void ULevelSequenceManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("LevelSequenceManagerSubsystem >> Initialized"));
}

void ULevelSequenceManagerSubsystem::Deinitialize()
{
    SequenceMap.Reset();

    Super::Deinitialize();
}


/* =====================================================================
   Registration
   ===================================================================== */

void ULevelSequenceManagerSubsystem::RegisterSequence(
    FName SequenceName, ALevelSequenceActor* SequenceActor)
{
    if (!SequenceActor || SequenceName.IsNone())
    {
        UE_LOG(LevelAreaGraphManagement, Warning,
            TEXT("LevelSequenceManagerSubsystem >> RegisterSequence: Invalid args"));
        return;
    }

    SequenceMap.Add(SequenceName, SequenceActor);

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("LevelSequenceManagerSubsystem >> Registered '%s'"), *SequenceName.ToString());
}

void ULevelSequenceManagerSubsystem::UnregisterSequence(FName SequenceName)
{
    SequenceMap.Remove(SequenceName);

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("LevelSequenceManagerSubsystem >> Unregistered '%s'"), *SequenceName.ToString());
}

void ULevelSequenceManagerSubsystem::RegisterBridge(ALevelAreaInstanceBridge* Bridge)
{
    if (!Bridge) return;

    UWorld* World = GetWorld();
    if (!World) return;

    AGameStateBase* GS = World->GetGameState();
    if (!GS) return;

    if (ULevelAreaGameStateComponent* GSComp =
        GS->FindComponentByClass<ULevelAreaGameStateComponent>())
    {
        GSComp->RegisterBridge(Bridge);
    }
    else
    {
        UE_LOG(LevelAreaGraphManagement, Warning,
            TEXT("LevelSequenceManagerSubsystem >> RegisterBridge: No ULevelAreaGameStateComponent on GameState"));
    }
}

void ULevelSequenceManagerSubsystem::UnregisterBridge(ALevelAreaInstanceBridge* Bridge)
{
    if (!Bridge) return;

    UWorld* World = GetWorld();
    if (!World) return;

    AGameStateBase* GS = World->GetGameState();
    if (!GS) return;

    if (ULevelAreaGameStateComponent* GSComp =
        GS->FindComponentByClass<ULevelAreaGameStateComponent>())
    {
        GSComp->UnregisterBridge(Bridge);
    }
}


/* =====================================================================
   Playback API — routes through replicator
   ===================================================================== */

void ULevelSequenceManagerSubsystem::PlaySequence(FName SequenceName)
{
    UWorld* World = GetWorld();
    if (!World) return;

    AGameStateBase* GS = World->GetGameState();
    if (!GS) return;

    if (ULevelSequenceReplicatorComponent* Replicator =
        GS->FindComponentByClass<ULevelSequenceReplicatorComponent>())
    {
        Replicator->Multicast_PlaySequence(SequenceName);
        return;
    }

    UE_LOG(LevelAreaGraphManagement, Warning,
        TEXT("LevelSequenceManagerSubsystem >> PlaySequence: No replicator on GameState"));
}

void ULevelSequenceManagerSubsystem::StopSequence(FName SequenceName)
{
    UWorld* World = GetWorld();
    if (!World) return;

    if (ULevelSequenceReplicatorComponent* Replicator =
        World->GetGameState()->FindComponentByClass<ULevelSequenceReplicatorComponent>())
    {
        Replicator->Multicast_StopSequence(SequenceName);
    }
}

void ULevelSequenceManagerSubsystem::PauseSequence(FName SequenceName)
{
    UWorld* World = GetWorld();
    if (!World) return;

    if (ULevelSequenceReplicatorComponent* Replicator =
        World->GetGameState()->FindComponentByClass<ULevelSequenceReplicatorComponent>())
    {
        Replicator->Multicast_PauseSequence(SequenceName);
    }
}

void ULevelSequenceManagerSubsystem::ResetSequence(FName SequenceName)
{
    UWorld* World = GetWorld();
    if (!World) return;

    if (ULevelSequenceReplicatorComponent* Replicator =
        World->GetGameState()->FindComponentByClass<ULevelSequenceReplicatorComponent>())
    {
        Replicator->Multicast_ResetSequence(SequenceName);
    }
}


/* =====================================================================
   Bridge-based Playback
   ===================================================================== */

void ULevelSequenceManagerSubsystem::PlaySequenceOnInstance(
    ALevelInstance* LevelInstance, FName SequenceName)
{
    ALevelAreaInstanceBridge* Bridge = GetBridgeByInstance(LevelInstance);
    if (!Bridge) return;

    ALevelSequenceActor* SeqActor = Bridge->GetSequenceByName(SequenceName);
    if (!SeqActor) return;

    RegisterSequence(SequenceName, SeqActor);
    PlaySequence(SequenceName);
}

void ULevelSequenceManagerSubsystem::StopSequenceOnInstance(
    ALevelInstance* LevelInstance, FName SequenceName)
{
    ALevelAreaInstanceBridge* Bridge = GetBridgeByInstance(LevelInstance);
    if (!Bridge) return;

    ALevelSequenceActor* SeqActor = Bridge->GetSequenceByName(SequenceName);
    if (!SeqActor) return;

    RegisterSequence(SequenceName, SeqActor);
    StopSequence(SequenceName);
}

void ULevelSequenceManagerSubsystem::PauseSequenceOnInstance(
    ALevelInstance* LevelInstance, FName SequenceName)
{
    ALevelAreaInstanceBridge* Bridge = GetBridgeByInstance(LevelInstance);
    if (!Bridge) return;

    ALevelSequenceActor* SeqActor = Bridge->GetSequenceByName(SequenceName);
    if (!SeqActor) return;

    RegisterSequence(SequenceName, SeqActor);
    PauseSequence(SequenceName);
}

void ULevelSequenceManagerSubsystem::ResetSequenceOnInstance(
    ALevelInstance* LevelInstance, FName SequenceName)
{
    ALevelAreaInstanceBridge* Bridge = GetBridgeByInstance(LevelInstance);
    if (!Bridge) return;

    ALevelSequenceActor* SeqActor = Bridge->GetSequenceByName(SequenceName);
    if (!SeqActor) return;

    RegisterSequence(SequenceName, SeqActor);
    ResetSequence(SequenceName);
}

void ULevelSequenceManagerSubsystem::PlaySequenceOnAllInstances(
    int32 NodeID, FName SequenceName)
{
    TArray<ALevelAreaInstanceBridge*> Bridges = GetBridgesByNodeID(NodeID);

    for (ALevelAreaInstanceBridge* Bridge : Bridges)
    {
        if (!Bridge) continue;

        ALevelSequenceActor* SeqActor = Bridge->GetSequenceByName(SequenceName);
        if (!SeqActor) continue;

        RegisterSequence(SequenceName, SeqActor);
        PlaySequence(SequenceName);
    }
}

void ULevelSequenceManagerSubsystem::StopSequenceOnAllInstances(
    int32 NodeID, FName SequenceName)
{
    TArray<ALevelAreaInstanceBridge*> Bridges = GetBridgesByNodeID(NodeID);

    for (ALevelAreaInstanceBridge* Bridge : Bridges)
    {
        if (!Bridge) continue;

        ALevelSequenceActor* SeqActor = Bridge->GetSequenceByName(SequenceName);
        if (!SeqActor) continue;

        RegisterSequence(SequenceName, SeqActor);
        StopSequence(SequenceName);
    }
}


/* =====================================================================
   Local — called by replicator on each client
   ===================================================================== */

void ULevelSequenceManagerSubsystem::PlaySequence_Local(FName SequenceName)
{
    ALevelSequenceActor* SeqActor = GetSequenceActor(SequenceName);
    if (!SeqActor) return;

    if (ULevelSequencePlayer* Player = SeqActor->GetSequencePlayer())
    {
        Player->Play();

        UE_LOG(LevelAreaGraphManagement, Log,
            TEXT("LevelSequenceManagerSubsystem >> Playing '%s'"), *SequenceName.ToString());
    }
}

void ULevelSequenceManagerSubsystem::StopSequence_Local(FName SequenceName)
{
    ALevelSequenceActor* SeqActor = GetSequenceActor(SequenceName);
    if (!SeqActor) return;

    if (ULevelSequencePlayer* Player = SeqActor->GetSequencePlayer())
    {
        Player->Stop();

        UE_LOG(LevelAreaGraphManagement, Log,
            TEXT("LevelSequenceManagerSubsystem >> Stopped '%s'"), *SequenceName.ToString());
    }
}

void ULevelSequenceManagerSubsystem::PauseSequence_Local(FName SequenceName)
{
    ALevelSequenceActor* SeqActor = GetSequenceActor(SequenceName);
    if (!SeqActor) return;

    if (ULevelSequencePlayer* Player = SeqActor->GetSequencePlayer())
    {
        Player->Pause();

        UE_LOG(LevelAreaGraphManagement, Log,
            TEXT("LevelSequenceManagerSubsystem >> Paused '%s'"), *SequenceName.ToString());
    }
}

void ULevelSequenceManagerSubsystem::ResetSequence_Local(FName SequenceName)
{
    ALevelSequenceActor* SeqActor = GetSequenceActor(SequenceName);
    if (!SeqActor) return;

    if (ULevelSequencePlayer* Player = SeqActor->GetSequencePlayer())
    {
        Player->Stop();

        FMovieSceneSequencePlaybackParams Params;
        Params.Time = 0.0;
        Params.PositionType = EMovieScenePositionType::Time;
        Player->SetPlaybackPosition(Params);

        UE_LOG(LevelAreaGraphManagement, Log,
            TEXT("LevelSequenceManagerSubsystem >> Reset '%s'"), *SequenceName.ToString());
    }
}

void ULevelSequenceManagerSubsystem::PlaySequenceAtTime_Local(
    FName SequenceName, float ServerStartTime)
{
    ALevelSequenceActor* SeqActor = GetSequenceActor(SequenceName);
    if (!SeqActor) return;

    ULevelSequencePlayer* Player = SeqActor->GetSequencePlayer();
    if (!Player) return;

    float CurrentServerTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
    float ElapsedTime = CurrentServerTime - ServerStartTime;

    float Duration = Player->GetDuration().AsSeconds();
    float LoopPosition = (Duration > 0.f) ? FMath::Fmod(ElapsedTime, Duration) : 0.f;

    FMovieSceneSequencePlaybackParams Params;
    Params.Time = FMath::Max(0.f, LoopPosition);
    Params.PositionType = EMovieScenePositionType::Time;
    Player->SetPlaybackPosition(Params);
    Player->Play();

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("LevelSequenceManagerSubsystem >> Resynced '%s' | Loop: %.2fs / %.2fs"),
        *SequenceName.ToString(), LoopPosition, Duration);
}


/* =====================================================================
   Query
   ===================================================================== */

ALevelSequenceActor* ULevelSequenceManagerSubsystem::GetSequenceActor(FName SequenceName) const
{
    TObjectPtr<ALevelSequenceActor> const* Found = SequenceMap.Find(SequenceName);

    if (!Found)
    {
        UE_LOG(LevelAreaGraphManagement, Warning,
            TEXT("LevelSequenceManagerSubsystem >> GetSequenceActor: '%s' not found"),
            *SequenceName.ToString());
        return nullptr;
    }

    return Found->Get();
}

ALevelAreaInstanceBridge* ULevelSequenceManagerSubsystem::GetBridgeByInstance(
    ALevelInstance* LevelInstance) const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    AGameStateBase* GS = World->GetGameState();
    if (!GS) return nullptr;

    ULevelAreaGameStateComponent* GameStateComp =
        GS->FindComponentByClass<ULevelAreaGameStateComponent>();
    if (!GameStateComp) return nullptr;

    return GameStateComp->GetBridgeActorByInstance(LevelInstance);
}

TArray<ALevelAreaInstanceBridge*> ULevelSequenceManagerSubsystem::GetBridgesByNodeID(
    int32 NodeID) const
{
    UWorld* World = GetWorld();
    if (!World) return {};

    AGameStateBase* GS = World->GetGameState();
    if (!GS) return {};

    ULevelAreaGameStateComponent* GameStateComp =
        GS->FindComponentByClass<ULevelAreaGameStateComponent>();
    if (!GameStateComp) return {};

    return GameStateComp->GetBridgeActorsByID(NodeID);
}