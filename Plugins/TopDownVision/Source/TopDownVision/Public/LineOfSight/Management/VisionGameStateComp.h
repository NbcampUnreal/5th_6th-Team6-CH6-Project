#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "LineOfSight/VisionData.h"
#include "VisionGameStateComp.generated.h"

class UVision_VisualComp;
class UVisionPlayerStateComp;

TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(VisionGameStateComp, Log, All);

// -------------------------------------------------------------------------- //
//  Fast Array
// -------------------------------------------------------------------------- //

USTRUCT()
struct FVisibleActorEntry : public FFastArraySerializerItem
{
    GENERATED_BODY()

    UPROPERTY()
    AActor* Target = nullptr;

    UPROPERTY()
    EVisionChannel TeamChannel = EVisionChannel::None;
};

USTRUCT()
struct FVisibleActorArray : public FFastArraySerializer
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FVisibleActorEntry> Items;

    UPROPERTY()
    UVisionGameStateComp* OwnerComp = nullptr;

    void PostReplicatedAdd   (const TArrayView<int32>& AddedIndices,   int32 FinalSize);
    void PreReplicatedRemove (const TArrayView<int32>& RemovedIndices, int32 FinalSize);
    void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize);

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FVisibleActorEntry, FVisibleActorArray>(
            Items, DeltaParms, *this);
    }
};

template<>
struct TStructOpsTypeTraits<FVisibleActorArray> : public TStructOpsTypeTraitsBase2<FVisibleActorArray>
{
    enum { WithNetDeltaSerializer = true };
};

// -------------------------------------------------------------------------- //
//  Pending entry — used when VisionPS isn't ready during a push callback
// -------------------------------------------------------------------------- //

struct FPendingVisibilityEntry
{
    TWeakObjectPtr<AActor> Target;
    EVisionChannel         Team    = EVisionChannel::None;
    bool                   bVisible = false;
};

// -------------------------------------------------------------------------- //
//  Component — pure replication pipe, no game logic
// -------------------------------------------------------------------------- //

UCLASS(ClassGroup=(Vision), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UVisionGameStateComp : public UActorComponent
{
    GENERATED_BODY()

public:
    UVisionGameStateComp();

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    // --- Server API --- //
    UFUNCTION(BlueprintCallable, Category="Vision")
    void SetActorVisibleToTeam(AActor* Target, EVisionChannel Team);

    UFUNCTION(BlueprintCallable, Category="Vision")
    void ClearActorVisibleToTeam(AActor* Target, EVisionChannel Team);

    UFUNCTION(BlueprintCallable, Category="Vision")
    bool IsActorVisibleToTeam(AActor* Target, EVisionChannel Team) const;

    // --- FastArray callbacks (client) --- //
    void OnTargetBecameVisible(AActor* Target, EVisionChannel Team);
    void OnTargetBecameHidden (AActor* Target, EVisionChannel Team);

    // --- Called by VisionPlayerStateComp::RefreshVisibility to drain queued entries --- //
    void FlushPendingReveals(UVisionPlayerStateComp* VisionPS);

    const TArray<FVisibleActorEntry>& GetVisibleActors() const { return VisibleActors.Items; }

private:
    UPROPERTY(Replicated)
    FVisibleActorArray VisibleActors;

    // Entries queued when VisionPS was null during a push callback
    TArray<FPendingVisibilityEntry> PendingReveals;
};