#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VisibilityMeshComp.generated.h"

TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(VisibilityMeshComp, Log, All);

class UMaterialInstanceDynamic;
class UMaterialInterface;
class USkeletalMeshComponent;
class UStaticMeshComponent;


/**
 * Manages visibility fade MIDs on the actor's tagged mesh components.
 *
 * Two operating modes:
 *
 *   Owned mode  — Initialize():
 *     Creates MIDs from whatever materials are already on the mesh slots.
 *     MIDs are owned for the actor's lifetime.
 *
 *   Pool mode   — SetMIDs():
 *     Receives pre-created MIDs from the pool subsystem on slot acquire.
 *     Applies them to the mesh slots directly, replacing existing materials.
 *     On slot release, SetMIDs({}) restores originals from OriginalMaterials cache.
 *
 * OriginalMaterials is populated by CacheOriginalMaterials(), called at the end
 * of FindMeshesByTag() and AddMesh() — always before any SetMaterial call.
 *
 * MeshKey identifies which FLOSVisibilityMeshMaterialSlot entry in
 * LOSResourcePoolSettings the pool uses to create MIDs for this component.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UVisibilityMeshComp : public UActorComponent
{
    GENERATED_BODY()

public:
    UVisibilityMeshComp();

public:

    /** Called in-editor to find tagged mesh components, populate targets,
     *  and cache original materials immediately. */
    UFUNCTION(BlueprintCallable, Category="Visibility")
    void FindMeshesByTag();

    /** Manually register a mesh. Call before Initialize.
     *  Automatically updates the original material cache. */
    void AddMesh(TSoftObjectPtr<USkeletalMeshComponent> Mesh);
    void AddMesh(TSoftObjectPtr<UStaticMeshComponent> Mesh);

    /** Owned mode — creates MIDs from the materials already on the mesh slots. */
    UFUNCTION(BlueprintCallable, Category="Visibility")
    void Initialize();

    /** Pool mode — receives pre-created MIDs from the subsystem and applies them
     *  to mesh slots. Pass an empty array to restore originals on slot release. */
    void SetMIDs(const TArray<TObjectPtr<UMaterialInstanceDynamic>>& InMIDs);

    /** Called every fade tick with the current alpha. */
    UFUNCTION(BlueprintCallable, Category="Visibility")
    void UpdateVisibility(float Alpha);

    UFUNCTION(BlueprintCallable, Category="Visibility")
    FName GetMeshKey() const { return MeshKey; }

    UFUNCTION(BlueprintCallable, Category="Visibility")
    void SetMeshKey(FName InMeshKey) { MeshKey = InMeshKey; }

protected:

    /** Identifies which FLOSVisibilityMeshMaterialSlot entry in LOSResourcePoolSettings
     *  the pool should use to create MIDs for this component.
     *  Must match a MeshKey in the settings array — NAME_None skips pool MIDs. */
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

    /** Snapshot of original materials before any SetMaterial call.
     *  Used to restore mesh slots on pool slot release.
     *  GetMaterial() after SetMaterial returns the MID not the original — never use it. */
    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInterface>> OriginalMaterials;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> MIDs;

private:
    /** Snapshots current materials. Must be called before any SetMaterial. */
    void CacheOriginalMaterials();

    /** Assigns InMIDs to mesh slots in order. Empty array calls RestoreOriginalMaterials. */
    void ApplyMIDsToMeshes(const TArray<TObjectPtr<UMaterialInstanceDynamic>>& InMIDs);

    /** Restores all mesh slots from OriginalMaterials cache. */
    void RestoreOriginalMaterials();
};
