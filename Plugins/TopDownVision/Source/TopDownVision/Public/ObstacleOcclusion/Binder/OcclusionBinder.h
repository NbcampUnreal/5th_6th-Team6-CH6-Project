#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ObstacleOcclusion/OcclusionTracer/OcclusionInterface.h"
#include "OcclusionBinder.generated.h"

class UMeshComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UOcclusionBinderSubsystem;

TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(OcclusionBinder, Log, All);

UCLASS()
class TOPDOWNVISION_API AOcclusionBinder : public AActor, public IOcclusionInterface
{
    GENERATED_BODY()

public:

    AOcclusionBinder();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;

    // ── Editor setup ──────────────────────────────────────────────────────

    UFUNCTION(CallInEditor, BlueprintCallable, Category="Occlusion Binder")
    void SetupBoundActors();

    // ── IOcclusionInterface ───────────────────────────────────────────────

    virtual void OnOcclusionEnter_Implementation(UObject* SourceTracer) override;
    virtual void OnOcclusionExit_Implementation(UObject* SourceTracer) override;
    virtual void ForceOcclude_Implementation(bool bForce) override;
    
    // ── Config ────────────────────────────────────────────────────────────

    // Actors to bind — populate in editor by dragging world actors here
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Binder")
    TArray<TObjectPtr<AActor>> BoundActors;

    // Tag for normal visible meshes — fades OUT when occluded
    // These meshes also get collision set for occlusion tracing
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Binder")
    FName NormalMeshTag = TEXT("OcclusionMesh");

    // Tag for occluded visual meshes — fades IN when occluded
    // These meshes are visual only — no collision
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Binder")
    FName OccludedMeshTag = TEXT("OccludedVisual");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Binder")
    float FadeSpeed = 6.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Binder")
    FName AlphaParameterName = TEXT("OcclusionAlpha");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Binder")
    TEnumAsByte<ECollisionChannel> OcclusionTraceChannel = ECC_GameTraceChannel1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Binder")
    TEnumAsByte<ECollisionChannel> MouseTraceChannel = ECC_Visibility;

    // Curved world proxy material — fully opaque, WPO driven by MPC
    UPROPERTY(EditAnywhere, Category="Occlusion Binder|Shadow")
    TObjectPtr<UMaterialInterface> ShadowProxyMaterial;

private:

    void DiscoverMeshes();
    void RegisterToSubsystem();
    void GenerateShadowProxies();
    void InitializeMaterials();
    void UpdateMaterialAlpha();
    void CleanupInvalidOverlaps();

    // Normal meshes — tagged NormalMeshTag — fade OUT when occluded (alpha = 1 - CurrentAlpha)
    // Serialized — populated at editor time
    UPROPERTY(VisibleAnywhere, Category="Occlusion Binder")
    TArray<TSoftObjectPtr<UMeshComponent>> NormalMeshes;

    // Occluded visual meshes — tagged OccludedMeshTag — fade IN when occluded (alpha = CurrentAlpha)
    // Visual only — no collision
    // Serialized — populated at editor time
    UPROPERTY(VisibleAnywhere, Category="Occlusion Binder")
    TArray<TSoftObjectPtr<UMeshComponent>> OccludedMeshes;

    // Shadow proxies — generated per bound actor at editor time
    UPROPERTY(VisibleAnywhere, Category="Occlusion Binder|Shadow")
    TArray<TObjectPtr<UStaticMeshComponent>> StaticShadowProxies;

    UPROPERTY(VisibleAnywhere, Category="Occlusion Binder|Shadow")
    TArray<TObjectPtr<USkeletalMeshComponent>> SkeletalShadowProxies;

    // MIDs — Transient, recreated at runtime
    UPROPERTY(Transient)
    TArray<UMaterialInstanceDynamic*> NormalDynamicMaterials;

    UPROPERTY(Transient)
    TArray<UMaterialInstanceDynamic*> OccludedDynamicMaterials;

    UPROPERTY(Transient)
    TSet<TWeakObjectPtr<UObject>> ActiveOverlaps;

    // Start at 1 — fully visible
    float CurrentAlpha      = 1.f;
    bool  bShouldBeOccluded = false;

    bool bForceOccluded = false;
};