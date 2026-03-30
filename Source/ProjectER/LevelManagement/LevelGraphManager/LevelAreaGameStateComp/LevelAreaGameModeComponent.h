#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LevelManagement/Requirements/LevelAreaData.h"
#include "LevelAreaGameModeComponent.generated.h"

class ULevelAreaGraphData;
class ALevelAreaInstanceBridge;
class ALevelInstance;


// BP exposed Delegate for danger zone area update
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPhaseChangedSignature, const TArray<int32>&, NewDangerZoneIDs);



UCLASS(ClassGroup = (LevelManagement), meta = (BlueprintSpawnableComponent))
class PROJECTER_API ULevelAreaGameModeComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    ULevelAreaGameModeComponent();

protected:

    virtual void BeginPlay() override;

public:

    void GenerateGraph();

    /* ---------- Config ---------- */

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hazard")
    bool bUseFixedSeed = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hazard")
    int32 HazardSeed = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hazard")
    int32 HazardsPerPhase = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hazard")
    EAreaHazardState HazardStatePerPhase = EAreaHazardState::Hazard;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LevelArea")
    ULevelAreaGraphData* GraphData;


    /* ---------- Replicated State ---------- */

    //!!!! Update -> The initial 0 will be nothing(safe) and the first phase will be also safe, no danger zone
    // Phase semantics:  
    // 0 = initial (safe)
    // 1 = safe phase (no hazards)
    // 2+ = hazards begin
    UPROPERTY(ReplicatedUsing = OnRep_HazardOrder, BlueprintReadOnly, Category = "Hazard")
    TArray<int32> HazardOrder;

    UPROPERTY(ReplicatedUsing = OnRep_CurrentPhase, BlueprintReadOnly, Category = "Hazard")
    int32 CurrentPhase = 0;


    /* ---------- Server API ---------- */

    UFUNCTION(BlueprintCallable, Category = "Hazard", BlueprintAuthorityOnly)
    void AdvancePhase();

    UFUNCTION(BlueprintCallable, Category = "Hazard", BlueprintAuthorityOnly)
    void SetPhase(int32 NewPhase);

    UFUNCTION(BlueprintCallable, Category = "Hazard", BlueprintAuthorityOnly)
    void ResetHazards(EAreaHazardState NewState = EAreaHazardState::None);

    // Instant death 
    UFUNCTION(BlueprintCallable, Category = "Hazard", BlueprintAuthorityOnly)
    void ScheduleInstantDeath(int32 NodeID, float Delay);

    UFUNCTION(BlueprintCallable, Category = "Hazard", BlueprintAuthorityOnly)
    void CancelInstantDeath(int32 NodeID);

    UFUNCTION(BlueprintCallable, Category = "Hazard", BlueprintAuthorityOnly)
    void ScheduleInstantDeathForAllHazards(float Delay);

    UFUNCTION(BlueprintCallable, Category = "Hazard", BlueprintAuthorityOnly)
    void CancelAllInstantDeath();


    /* ---------- Getters ---------- */

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Hazard")
    int32 GetHazardSeed() const { return HazardSeed; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Hazard")
    int32 GetTotalPhases() const
    {
        int32 HazardPhases = HazardOrder.Num() / FMath::Max(1, HazardsPerPhase);
        return HazardPhases + 1; // +1 for safe phase
    }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Hazard")
    bool IsAllPhasesExhausted() const
    {
        int32 EffectivePhase = FMath::Max(0, CurrentPhase - 1);
        return (EffectivePhase * HazardsPerPhase) >= HazardOrder.Num();
    }

    // Wide — all bridges for a node area
    UFUNCTION(BlueprintCallable, Category = "LevelArea")
    TArray<ALevelAreaInstanceBridge*> GetBridgeActorsByID(int32 NodeID) const;

    // Narrow — exact bridge by its linked level instance
    UFUNCTION(BlueprintCallable, Category = "LevelArea")
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

    // NodeID → active timer handle
    TMap<int32, FTimerHandle> InstantDeathTimerMap;

    // Client-side last applied phase for replication catch-up
    UPROPERTY()
    int32 LastAppliedPhase = 0;


    UFUNCTION()
    void OnRep_HazardOrder();

    UFUNCTION()
    void OnRep_CurrentPhase();

    // Core hazard apply — explicit phase and state instead of reading member vars
    void ApplyHazards(int32 Phase, EAreaHazardState State);

    // Notify bridges for a list of nodes with an explicit state
    void NotifyBridgeActors(const TArray<int32>& NodeIDs, EAreaHazardState State);

    void NotifyTrackers();
    void BuildBridgeActorMap();

    void ApplyInstantDeathToNode(int32 NodeID);


#pragma region MPC update

public:
    
    UPROPERTY(BlueprintAssignable, Category="LineOfSight")
    FOnPhaseChangedSignature OnPhaseChanged;

#pragma endregion
};