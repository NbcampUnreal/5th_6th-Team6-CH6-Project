#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LevelAreaTrackerComponent.generated.h"

UENUM(BlueprintType)
enum class EAreaHazardState : uint8
{
    Safe,
    Hazard,
    InstantDeath
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNodeChanged, int32, OldNodeID, int32, NewNodeID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHazardStateChanged, EAreaHazardState, OldState, EAreaHazardState, NewState);

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
    FOnNodeChanged OnNodeChanged;

    UPROPERTY(BlueprintAssignable, Category="Area")
    FOnHazardStateChanged OnHazardStateChanged;


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