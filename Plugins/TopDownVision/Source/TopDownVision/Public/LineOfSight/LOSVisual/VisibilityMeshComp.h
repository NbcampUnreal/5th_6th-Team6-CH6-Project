#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VisibilityMeshComp.generated.h"

TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(VisibilityMeshComp, Log, All);

class UMaterialInstanceDynamic;
class USkeletalMeshComponent;
class UStaticMeshComponent;


/**
 * Manages visibility fade MIDs on the actor's tagged mesh components.
 * MIDs are always locally owned — never pooled.
 *
 * Because base character loads mesh data at runtime, call SetMeshKey()
 * after monster data loads — it triggers FindMeshesByTag() internally
 * so mesh targets are resolved before Initialize() creates MIDs.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UVisibilityMeshComp : public UActorComponent
{
    GENERATED_BODY()

public:
    UVisibilityMeshComp();

public:

    /** Called in-editor or via SetMeshKey() at runtime to find tagged meshes. */
    UFUNCTION(BlueprintCallable, Category="Visibility")
    void FindMeshesByTag();

    void AddMesh(TSoftObjectPtr<USkeletalMeshComponent> Mesh);
    void AddMesh(TSoftObjectPtr<UStaticMeshComponent> Mesh);

    /** Creates MIDs from materials already on the mesh slots.
     *  Call after FindMeshesByTag() or SetMeshKey(). */
    UFUNCTION(BlueprintCallable, Category="Visibility")
    void Initialize();

    UFUNCTION(BlueprintCallable, Category="Visibility")
    void UpdateVisibility(float Alpha);

    UFUNCTION(BlueprintCallable, Category="Visibility")
    FName GetMeshKey() const { return MeshKey; }

    /** Set key and trigger FindMeshesByTag() — call after monster data loads at runtime. */
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

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> MIDs;
};
