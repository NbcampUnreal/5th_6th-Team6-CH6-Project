#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "ObstacleOcclusion/OcclusionInterface.h"
#include "OcclusionObstacleComp_Physical.generated.h"

class UMeshComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UOcclusionObstacleComp_Physical : public USceneComponent, public IOcclusionInterface
{
    GENERATED_BODY()

public:
    UOcclusionObstacleComp_Physical();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // Discovers child meshes and generates shadow proxies — call in editor via CallInEditor
    UFUNCTION(BlueprintCallable, Category="Occlusion Setup")
    void SetupOcclusionMeshes();

    // Sets collision and shadow settings on discovered meshes
    UFUNCTION(BlueprintCallable, Category="Occlusion Setup")
    void InitializeCollisionAndShadow();

    virtual void OnOcclusionEnter_Implementation(UObject* SourceTracer) override;
    virtual void OnOcclusionExit_Implementation(UObject* SourceTracer) override;

private:

    // Creates hidden mesh proxies for shadow casting
    // Static: UStaticMeshComponent proxy
    // Skeletal: USkeletalMeshComponent proxy with SetLeaderPoseComponent for anim mirroring
    void GenerateShadowProxyMeshes();

    void InitializeMaterials();     // MID creation at runtime
    void UpdateMaterialAlpha();     // sends visibility alpha to MIDs each tick
    void CleanupInvalidOverlaps();
    void DiscoverChildMeshes();     // finds tagged meshes from owner actor hierarchy

protected:

    UPROPERTY(EditAnywhere, Category="Occlusion")
    FName NormalMeshTag = TEXT("OcclusionMesh");

    UPROPERTY(EditAnywhere, Category="Occlusion")
    FName OccludedMeshTag = TEXT("OccludedVisual");

    UPROPERTY(EditAnywhere, Category="Occlusion")
    float FadeSpeed = 6.f;

    UPROPERTY(EditAnywhere, Category="Occlusion")
    FName AlphaParameterName = TEXT("OcclusionAlpha");

    UPROPERTY(EditAnywhere, Category="Occlusion")
    TEnumAsByte<ECollisionChannel> OcclusionTraceChannel = ECC_GameTraceChannel1;

    UPROPERTY(EditAnywhere, Category="Occlusion")
    TEnumAsByte<ECollisionChannel> MouseTraceChannel = ECC_Visibility;

    // Curved world proxy material — fully opaque, WPO driven by MPC
    // Used for all shadow proxies (static and skeletal) — no MID needed
    UPROPERTY(EditAnywhere, Category="Occlusion|Shadow")
    TObjectPtr<UMaterialInterface> ShadowProxyMaterial;

private:
    UPROPERTY(Transient)
    TSet<TWeakObjectPtr<UObject>> ActiveOverlaps;

    float CurrentAlpha        = 0.f;
    bool  bShouldBeOccluded   = false;
    bool  bLastOcclusionState = false;

    UPROPERTY(Transient)
    TArray<UMaterialInstanceDynamic*> NormalDynamicMaterials;

    UPROPERTY(Transient)
    TArray<UMaterialInstanceDynamic*> OccludedDynamicMaterials;

    // UMeshComponent covers both static and skeletal meshes
    UPROPERTY(VisibleAnywhere)
    TArray<TSoftObjectPtr<UMeshComponent>> NormalMeshes;

    UPROPERTY(VisibleAnywhere)
    TArray<TSoftObjectPtr<UMeshComponent>> OccludedMeshes;

    // Static mesh shadow proxies
    UPROPERTY(VisibleAnywhere, Category="Occlusion|Shadow")
    TArray<TObjectPtr<UStaticMeshComponent>> StaticShadowProxies;

    // Skeletal mesh shadow proxies — mirrors animations via SetLeaderPoseComponent
    UPROPERTY(VisibleAnywhere, Category="Occlusion|Shadow")
    TArray<TObjectPtr<USkeletalMeshComponent>> SkeletalShadowProxies;
};