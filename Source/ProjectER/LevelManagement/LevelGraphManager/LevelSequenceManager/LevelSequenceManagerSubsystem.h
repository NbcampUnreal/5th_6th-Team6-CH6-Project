#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "LevelSequenceManagerSubsystem.generated.h"

class ALevelSequenceActor;
class ALevelAreaInstanceBridge;

UCLASS()
class PROJECTER_API ULevelSequenceManagerSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;


    /* ---------- Registration ---------- */

    UFUNCTION(BlueprintCallable, Category="SequenceManager")
    void RegisterSequence(FName SequenceName, ALevelSequenceActor* SequenceActor);

    UFUNCTION(BlueprintCallable, Category="SequenceManager")
    void UnregisterSequence(FName SequenceName);

    // Bridge registration — forwards to game state component map
    UFUNCTION(BlueprintCallable, Category="SequenceManager")
    void RegisterBridge(ALevelAreaInstanceBridge* Bridge);

    UFUNCTION(BlueprintCallable, Category="SequenceManager")
    void UnregisterBridge(ALevelAreaInstanceBridge* Bridge);

    
    /* ---------- Playback API — routes through replicator → all clients ---------- */

    UFUNCTION(BlueprintCallable, Category="SequenceManager")
    void PlaySequence(FName SequenceName);

    UFUNCTION(BlueprintCallable, Category="SequenceManager")
    void StopSequence(FName SequenceName);

    UFUNCTION(BlueprintCallable, Category="SequenceManager")
    void PauseSequence(FName SequenceName);

    UFUNCTION(BlueprintCallable, Category="SequenceManager")
    void ResetSequence(FName SequenceName);


    /* ---------- Bridge-based Playback ---------- */

    // Narrow — specific sequence on a specific level instance
    UFUNCTION(BlueprintCallable, Category="SequenceManager")
    void PlaySequenceOnInstance(ALevelInstance* LevelInstance, FName SequenceName);

    UFUNCTION(BlueprintCallable, Category="SequenceManager")
    void StopSequenceOnInstance(ALevelInstance* LevelInstance, FName SequenceName);

    UFUNCTION(BlueprintCallable, Category="SequenceManager")
    void PauseSequenceOnInstance(ALevelInstance* LevelInstance, FName SequenceName);

    UFUNCTION(BlueprintCallable, Category="SequenceManager")
    void ResetSequenceOnInstance(ALevelInstance* LevelInstance, FName SequenceName);

    // Wide — same sequence on all instances sharing a NodeID
    UFUNCTION(BlueprintCallable, Category="SequenceManager")
    void PlaySequenceOnAllInstances(int32 NodeID, FName SequenceName);

    UFUNCTION(BlueprintCallable, Category="SequenceManager")
    void StopSequenceOnAllInstances(int32 NodeID, FName SequenceName);


    /* ---------- Local — called by replicator on each client ---------- */

    void PlaySequence_Local(FName SequenceName);
    void StopSequence_Local(FName SequenceName);
    void PauseSequence_Local(FName SequenceName);
    void ResetSequence_Local(FName SequenceName);

    // Resyncs a looping sequence to the correct position using server start time
    void PlaySequenceAtTime_Local(FName SequenceName, float ServerStartTime);


    /* ---------- Query ---------- */

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="SequenceManager")
    ALevelSequenceActor* GetSequenceActor(FName SequenceName) const;

    // Narrow — find exactly one bridge by its level instance
    UFUNCTION(BlueprintCallable, Category="SequenceManager")
    ALevelAreaInstanceBridge* GetBridgeByInstance(ALevelInstance* LevelInstance) const;

    // Wide — find all bridges for a node area
    UFUNCTION(BlueprintCallable, Category="SequenceManager")
    TArray<ALevelAreaInstanceBridge*> GetBridgesByNodeID(int32 NodeID) const;

private:

    // Naming convention: Area{NodeID}_{SequenceName} e.g. "Area2_Collapse"
    TMap<FName, TObjectPtr<ALevelSequenceActor>> SequenceMap;
};