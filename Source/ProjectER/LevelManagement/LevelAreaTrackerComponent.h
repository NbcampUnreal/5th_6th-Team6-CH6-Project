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

    UFUNCTION(BlueprintCallable, Category="Area")
    void UpdateArea();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="Area")
    EAreaHazardState GetHazardState() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="Area")
    bool IsInHazardArea() const { return CurrentHazardState != EAreaHazardState::Safe; }

    UFUNCTION(BlueprintCallable, Category="Area")
    void RefreshHazardState();


    /* ---------- Lifecycle ---------- */

    virtual void BeginPlay() override;


private:

    EAreaHazardState CachedHazardState = EAreaHazardState::Safe;
};