#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LevelAreaGameStateComponent.generated.h"

/*
 * Attach to AGameStateBase.
 * Server drives phase progression and generates hazard order.
 * HazardNodeIDs replicates to all clients, which then notify their local subsystem.
 */
UCLASS(ClassGroup=(LevelManagement), meta=(BlueprintSpawnableComponent))
class PROJECTER_API ULevelAreaGameStateComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    ULevelAreaGameStateComponent();


    /* ---------- Config ---------- */

    // How many areas become hazardous per phase tick
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hazard")
    int32 HazardsPerPhase = 1;


    /* ---------- Replicated State ---------- */

    // Full current hazard set — replicated so all clients can mirror it
    UPROPERTY(ReplicatedUsing=OnRep_HazardNodeIDs, BlueprintReadOnly, Category="Hazard")
    TArray<int32> HazardNodeIDs;

    UPROPERTY(BlueprintReadOnly, Category="Hazard")
    int32 CurrentPhase = 0;


    /* ---------- Server API ---------- */

    /*
     * Call this on the server each time the game advances a phase.
     * Generates the next batch of hazard nodes, replicates them,
     * and notifies the matching LevelAreaComponents.
     */
    UFUNCTION(BlueprintCallable, Category="Hazard", BlueprintAuthorityOnly)
    void AdvancePhase();

    // Reset everything (e.g. round restart)
    UFUNCTION(BlueprintCallable, Category="Hazard", BlueprintAuthorityOnly)
    void ResetHazards();


    /* ---------- Lifecycle ---------- */

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;


private:

    // Ordered sequence generated at phase start — server only
    TArray<int32> HazardOrder;

    UFUNCTION()
    void OnRep_HazardNodeIDs();

    // Server: find every LevelAreaComponent and set its hazard flag
    void ApplyHazardToComponents(const TArray<int32>& NodeIDs, bool bHazard);

    // Push current HazardNodeIDs into the local subsystem
    void NotifySubsystem(const TArray<int32>& NodeIDs);
};