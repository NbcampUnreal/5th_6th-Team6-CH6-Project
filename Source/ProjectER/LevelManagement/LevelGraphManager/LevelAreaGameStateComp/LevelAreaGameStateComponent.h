#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LevelManagement/Requirements/LevelAreaData.h"
#include "LevelAreaGameStateComponent.generated.h"

class ULevelAreaGraphData;

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

    // Full collapse order — replicated once at start, never changes
    UPROPERTY(ReplicatedUsing=OnRep_HazardOrder, BlueprintReadOnly, Category="Hazard")
    TArray<int32> HazardOrder;

    // Current phase — clients derive active hazards from this + HazardOrder
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


    /* ---------- Lifecycle ---------- */

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;


private:

    UFUNCTION()
    void OnRep_HazardOrder();

    UFUNCTION()
    void OnRep_CurrentPhase();

    // Applies all hazards up to and including CurrentPhase from HazardOrder
    void ApplyHazardsUpToCurrentPhase();

    void NotifyTrackers();
};