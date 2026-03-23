#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LevelManagement/Requirements/LevelAreaData.h"
#include "LevelAreaGameStateComponent.generated.h"

class ULevelAreaGraphData;
class ALevelAreaInstanceBridge;
class ALevelInstance;

UCLASS(ClassGroup=(LevelManagement), meta=(BlueprintSpawnableComponent))
class PROJECTER_API ULevelAreaGameStateComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    ULevelAreaGameStateComponent();

protected:

    virtual void BeginPlay() override;

public:

    /* ---------- Config ---------- */

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hazard")
    int32 HazardsPerPhase = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hazard")
    EAreaHazardState HazardStatePerPhase = EAreaHazardState::Hazard;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hazard")
    int32 HazardSeed = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LevelArea")
    ULevelAreaGraphData* GraphData;


    /* ---------- Replicated State ---------- */

    UPROPERTY(ReplicatedUsing=OnRep_HazardOrder, BlueprintReadOnly, Category="Hazard")
    TArray<int32> HazardOrder;

    UPROPERTY(ReplicatedUsing=OnRep_CurrentPhase, BlueprintReadOnly, Category="Hazard")
    int32 CurrentPhase = 0;


    /* ---------- Server API ---------- */

    UFUNCTION(BlueprintCallable, Category="Hazard", BlueprintAuthorityOnly)
    void AdvancePhase();

    UFUNCTION(BlueprintCallable, Category="Hazard", BlueprintAuthorityOnly)
    void ResetHazards();


    /* ---------- Getters ---------- */

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="Hazard")
    int32 GetHazardSeed() const { return HazardSeed; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="Hazard")
    int32 GetTotalPhases() const { return HazardOrder.Num() / FMath::Max(1, HazardsPerPhase); }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="Hazard")
    bool IsAllPhasesExhausted() const { return (CurrentPhase * HazardsPerPhase) >= HazardOrder.Num(); }

    // Wide — all bridges for a node area
    UFUNCTION(BlueprintCallable, Category="LevelArea")
    TArray<ALevelAreaInstanceBridge*> GetBridgeActorsByID(int32 NodeID) const;

    // Narrow — exact bridge by its linked level instance
    UFUNCTION(BlueprintCallable, Category="LevelArea")
    ALevelAreaInstanceBridge* GetBridgeActorByInstance(ALevelInstance* LevelInstance) const;


    /* ---------- Lifecycle ---------- */

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /// registration
    void RegisterBridge(ALevelAreaInstanceBridge* Bridge);
    void UnregisterBridge(ALevelAreaInstanceBridge* Bridge);

public:

    TMap<int32, TArray<TObjectPtr<ALevelAreaInstanceBridge>>> BridgeActorMap;

private:

    UFUNCTION()
    void OnRep_HazardOrder();

    UFUNCTION()
    void OnRep_CurrentPhase();

    void ApplyHazardsUpToCurrentPhase();
    void NotifyTrackers();
    void BuildBridgeActorMap();
    void NotifyBridgeActors(const TArray<int32>& HazardNodeIDs);
};