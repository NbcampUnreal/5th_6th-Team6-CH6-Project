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
    virtual void BeginPlay() override;// now load the graph using the 

public:
    /* ---------- Config ---------- */

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hazard")
    int32 HazardsPerPhase = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hazard")
    EAreaHazardState HazardStatePerPhase = EAreaHazardState::Hazard;


    /* ---------- Replicated State ---------- */

    UPROPERTY(ReplicatedUsing=OnRep_HazardNodeIDs, BlueprintReadOnly, Category="Hazard")
    TArray<int32> HazardNodeIDs;

    UPROPERTY(BlueprintReadOnly, Category="Hazard")
    int32 CurrentPhase = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hazard")
    int32 HazardSeed = 0;// for random



    /* -------- Graph Data Asset -------*/
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LevelArea")
    ULevelAreaGraphData* GraphData;


    /* ---------- Server API ---------- */

    UFUNCTION(BlueprintCallable, Category="Hazard", BlueprintAuthorityOnly)
    void AdvancePhase();

    UFUNCTION(BlueprintCallable, Category="Hazard", BlueprintAuthorityOnly)
    void ResetHazards();


    /* ---------- Lifecycle ---------- */

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;


    /* ----- Getter for the seed -----*/
    UFUNCTION(BlueprintCallable, Category="Hazard")
    int32 GetHazardSeed() const { return HazardSeed; }

private:

    TArray<int32> HazardOrder;

    UFUNCTION()
    void OnRep_HazardNodeIDs();

    void NotifySubsystem(const TArray<int32>& NodeIDs);
};