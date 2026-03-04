// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "LineOfSight/VisionData.h"
#include "VisionGameStateComp.generated.h"

class UVision_VisualComp;

TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(VisionGameStateComp, Log, All);

// -------------------------------------------------------------------------- //
//  Fast Array Entry
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

// -------------------------------------------------------------------------- //
//  Fast Array Container
// -------------------------------------------------------------------------- //

USTRUCT()
struct FVisibleActorArray : public FFastArraySerializer
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FVisibleActorEntry> Items;
    
    UPROPERTY()
    UVisionGameStateComp* OwnerComp = nullptr;

    void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize);
    void PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize); // fires BEFORE removal
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
//  Component
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

    /** Called by Vision_EvaluatorComp on server to report a target as visible to a team. */
    UFUNCTION(BlueprintCallable, Category="Vision")
    void SetActorVisibleToTeam(AActor* Target, EVisionChannel Team);

    /** Called by Vision_EvaluatorComp on server when no observer can see the target. */
    UFUNCTION(BlueprintCallable, Category="Vision")
    void ClearActorVisibleToTeam(AActor* Target, EVisionChannel Team);

    /** Returns true if the target is currently visible to the given team. */
    UFUNCTION(BlueprintCallable, Category="Vision")
    bool IsActorVisibleToTeam(AActor* Target, EVisionChannel Team) const;

    // --- Called by FastArray callbacks on clients --- //
    void OnTargetBecameVisible(AActor* Target, EVisionChannel Team);
    void OnTargetBecameHidden(AActor* Target, EVisionChannel Team);

    /** Called by LOSVisionSubsystem when a new provider registers.
    *  Reveals all existing same-team providers to each other automatically. */
    void OnProviderRegistered(UVision_VisualComp* NewProvider, EVisionChannel Channel);

    const TArray<FVisibleActorEntry>& GetVisibleActors() const { return VisibleActors.Items; }

private:
    UPROPERTY(Replicated)
    FVisibleActorArray VisibleActors;
};