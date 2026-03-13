#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LevelManagement/Requirements/LevelAreaData.h"
#include "LevelAreaGameStateComponent.generated.h"

UCLASS(ClassGroup=(LevelManagement), meta=(BlueprintSpawnableComponent))
class PROJECTER_API ULevelAreaGameStateComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    ULevelAreaGameStateComponent();


    /* ---------- Config ---------- */

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hazard")
    int32 HazardsPerPhase = 1;

    // State applied to newly hazardous nodes each phase
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hazard")
    EAreaHazardState HazardStatePerPhase = EAreaHazardState::Hazard;


    /* ---------- Replicated State ---------- */

    UPROPERTY(ReplicatedUsing=OnRep_HazardNodeIDs, BlueprintReadOnly, Category="Hazard")
    TArray<int32> HazardNodeIDs;

    UPROPERTY(BlueprintReadOnly, Category="Hazard")
    int32 CurrentPhase = 0;


    /* ---------- Server API ---------- */

    UFUNCTION(BlueprintCallable, Category="Hazard", BlueprintAuthorityOnly)
    void AdvancePhase();

    UFUNCTION(BlueprintCallable, Category="Hazard", BlueprintAuthorityOnly)
    void ResetHazards();


    /* ---------- Lifecycle ---------- */

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;


private:

    TArray<int32> HazardOrder;

    UFUNCTION()
    void OnRep_HazardNodeIDs();

    void NotifySubsystem(const TArray<int32>& NodeIDs);
};