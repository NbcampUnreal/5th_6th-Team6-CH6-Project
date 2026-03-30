#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VisibilityMeshComp.generated.h"

TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(VisibilityMeshComp, Log, All);

class UMaterialInstanceDynamic;
class USkeletalMeshComponent;
class UStaticMeshComponent;
struct FLOSVisibilityMIDSet;


/**
 * Manages visibility fade MIDs on the actor's tagged mesh components.
 *
 * Pool mode — SetMIDsFromPool() / ClearPoolMIDs():
 *   Receives a pre-created FLOSVisibilityMIDSet from the pool subsystem.
 *   Each MID is applied to its specified slot index directly.
 *   On release, ClearPoolMIDs() restores the original material on each slot.
 *
 * Owned mode — Initialize():
 *   Creates MIDs from whatever materials are on the mesh slots.
 *   Used when no matching MeshKey found in settings or pool is exhausted.
 *
 * Call SetMeshKey() after monster data loads at runtime — triggers FindMeshesByTag()
 * and caches original materials before any SetMaterial call.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UVisibilityMeshComp : public UActorComponent
{
    GENERATED_BODY()

public:
    UVisibilityMeshComp();

public:

    UFUNCTION(BlueprintCallable, Category="Visibility")
    void FindMeshesByTag();

    void AddMesh(TSoftObjectPtr<USkeletalMeshComponent> Mesh);
    void AddMesh(TSoftObjectPtr<UStaticMeshComponent> Mesh);

    /** Owned mode — creates MIDs from materials already on mesh slots. */
    UFUNCTION(BlueprintCallable, Category="Visibility")
    void Initialize();

    /** Pool mode — applies a pre-created MID set by slot index.
     *  Called by the pool subsystem on slot acquire. */
    void SetMIDsFromPool(const FLOSVisibilityMIDSet& InSet);

    /** Pool mode — restores original materials and clears active MIDs.
     *  Called by the pool subsystem on slot release. */
    void ClearPoolMIDs();

    UFUNCTION(BlueprintCallable, Category="Visibility")
    void UpdateVisibility(float Alpha);

    UFUNCTION(BlueprintCallable, Category="Visibility")
    FName GetMeshKey() const { return MeshKey; }

    /** Set key and trigger FindMeshesByTag() — call after monster data loads. */
    UFUNCTION(BlueprintCallable, Category="Visibility")
    void SetMeshKey(FName InMeshKey);

protected:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visibility")
    FName MeshKey = NAME_None;

    UPROPERTY(EditAnywhere, Category="Visibility")
    FName VisibilityMeshTag = TEXT("VisibilityMesh");

    UPROPERTY(VisibleAnywhere, Category="Visibility")
    TArray<TSoftObjectPtr<USkeletalMeshComponent>> SkeletalMeshTargets;

    UPROPERTY(VisibleAnywhere, Category="Visibility")
    TArray<TSoftObjectPtr<UStaticMeshComponent>> StaticMeshTargets;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visibility")
    FName VisibilityParam = TEXT("VisibilityAlpha");

private:

    /** Original material per slot — cached before first SetMaterial call. */
    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInterface>> OriginalMaterials;

    /** Slot indices parallel to OriginalMaterials — which mesh slot each original belongs to. */
    TArray<int32> OriginalSlotIndices;

    /** Active MIDs — owned (Initialize) or pooled (SetMIDsFromPool). */
    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> MIDs;

    /** Whether active MIDs came from the pool (true) or are locally owned (false). */
    bool bMIDsArePooled = false;

private:
    void CacheOriginalMaterials();
    UMeshComponent* FindMeshComponentForSlot(int32 GlobalSlotIndex, int32& OutLocalSlotIndex) const;
};
