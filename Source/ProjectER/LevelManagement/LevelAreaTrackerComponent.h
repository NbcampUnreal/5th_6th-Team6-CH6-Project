#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LevelManagement/Requirements/LevelAreaData.h"
#include "LevelAreaTrackerComponent.generated.h"

UCLASS(ClassGroup=(LevelManagement), meta=(BlueprintSpawnableComponent))
class PROJECTER_API ULevelAreaTrackerComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    ULevelAreaTrackerComponent();

    /* ---------- Config ---------- */

    UPROPERTY(EditAnywhere, Category="Area")
    float TraceLength = 150.f;

    UPROPERTY(EditAnywhere, Category="Area")
    TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;


    /* ---------- State ---------- */

    UPROPERTY(BlueprintReadOnly, Category="Area")
    int32 CurrentNodeID = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, Category="Area")
    EAreaHazardState CurrentHazardState = EAreaHazardState::Safe;


    /* ---------- Delegates ---------- */

    UPROPERTY(BlueprintAssignable, Category="Area")
    FOnAreaNodeChanged OnNodeChanged;

    UPROPERTY(BlueprintAssignable, Category="Area")
    FOnAreaHazardStateTransition OnHazardStateChanged;


    /* ---------- API ---------- */

    // Main update — traces floor, updates CurrentNodeID, refreshes hazard state
    UFUNCTION(BlueprintCallable, Category="Area")
    void UpdateArea();

    // Returns current hazard state of CurrentNodeID
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="Area")
    EAreaHazardState GetHazardState() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="Area")
    bool IsInHazardArea() const { return CurrentHazardState != EAreaHazardState::Safe; }

    // Push-based refresh — called from GameStateComponent when
    // a room becomes hazardous under a standing player
    UFUNCTION(BlueprintCallable, Category="Area")
    void RefreshHazardState();


    /* ---------- Debug ---------- */

    // Performs the downward trace and returns the raw hit NodeID without updating state
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="Area|Debug")
    int32 TraceForNodeID() const;

    // Queries the subsystem for the hazard state of any given NodeID
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="Area|Debug")
    EAreaHazardState GetHazardStateForNode(int32 InNodeID) const;


    /* ---------- Lifecycle ---------- */

    virtual void BeginPlay() override;


private:

    EAreaHazardState CachedHazardState = EAreaHazardState::Safe;
};