#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LevelManagement/Area/LevelAreaNode.h"
#include "LevelAreaComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHazardStateChanged, bool, bIsHazard);

/*
 * Attach to every room actor (ALevelArea or similar).
 * - Server: creates and registers the LevelAreaNode with the subsystem.
 * - Replicates bIsHazard to clients for visual / audio feedback.
 * - Tracks which pawns are inside via overlap so they can query hazard state.
 */
UCLASS(ClassGroup=(LevelManagement), meta=(BlueprintSpawnableComponent))
class PROJECTER_API ULevelAreaComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    ULevelAreaComponent();


    /* ---------- Config (set per room in editor) ---------- */

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area")
    int32 NodeID = INDEX_NONE;

    // Adjacent room IDs — set these up to match your graph topology
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area")
    TArray<int32> NeighborNodeIDs;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area")
    FVector RoomCenter = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Area")
    FVector RoomExtent = FVector(500.f, 500.f, 200.f);


    /* ---------- Runtime State ---------- */

    UPROPERTY(ReplicatedUsing=OnRep_bIsHazard, BlueprintReadOnly, Category="Area")
    bool bIsHazard = false;

    UPROPERTY(BlueprintAssignable, Category="Area")
    FOnHazardStateChanged OnHazardStateChanged;


    /* ---------- Server API ---------- */

    // Called by GameStateComponent when this node becomes a hazard
    UFUNCTION(BlueprintCallable, Category="Area")
    void SetHazard(bool bNewHazard);

    // Which player pawns are currently inside this room
    UFUNCTION(BlueprintCallable, Category="Area")
    const TArray<APawn*>& GetPawnsInside() const { return PawnsInside; }


    /* ---------- Overlap Registration ---------- */

    // Bind your room's overlap volume to these in BeginPlay
    UFUNCTION()
    void OnPawnEntered(APawn* Pawn);

    UFUNCTION()
    void OnPawnExited(APawn* Pawn);


    /* ---------- Lifecycle ---------- */

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;


private:

    // Owned node — lifetime managed by this component
    TUniquePtr<LevelAreaNode> OwnedNode;

    UPROPERTY()
    TArray<APawn*> PawnsInside;// tracks what pawn is in the area

    UFUNCTION()
    void OnRep_bIsHazard();

    void RegisterWithSubsystem();
    void UnregisterFromSubsystem();
};