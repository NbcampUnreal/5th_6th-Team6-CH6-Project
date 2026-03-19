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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Binder")
    TArray<TObjectPtr<AActor>> BoundActors;

    // Tag for normal visible meshes — fades OUT when occluded
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Binder")
    FName NormalMeshTag = TEXT("OcclusionMesh");

    // Tag for occluded visual meshes — fades IN when occluded
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Binder")
    FName OccludedMeshTag = TEXT("OccludedVisual");

    // Tag for meshes using RT-based material — gets ForceOccluded parameter
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Binder")
    FName RTMaterialTag = TEXT("RTOcclusionMesh");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Binder")
    float FadeSpeed = 6.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Binder")
    FName AlphaParameterName = TEXT("OcclusionAlpha");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Binder")
    FName ForceOccludeParameterName = TEXT("ForceOccluded");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Binder")
    TEnumAsByte<ECollisionChannel> OcclusionTraceChannel = ECC_GameTraceChannel1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Binder")
    TEnumAsByte<ECollisionChannel> MouseTraceChannel = ECC_Visibility;

    UPROPERTY(EditAnywhere, Category="Occlusion Binder|Shadow")
    TObjectPtr<UMaterialInterface> ShadowProxyMaterial;

private:

    void DiscoverMeshes();
    void RegisterToSubsystem();
    void GenerateShadowProxies();
    void InitializeMaterials();
    void UpdateMaterialAlpha();
    void CleanupInvalidOverlaps();

    UPROPERTY(VisibleAnywhere, Category="Occlusion Binder")
    TArray<TSoftObjectPtr<UMeshComponent>> NormalMeshes;

    UPROPERTY(VisibleAnywhere, Category="Occlusion Binder")
    TArray<TSoftObjectPtr<UMeshComponent>> OccludedMeshes;

    UPROPERTY(VisibleAnywhere, Category="Occlusion Binder|Shadow")
    TArray<TObjectPtr<UStaticMeshComponent>> StaticShadowProxies;

    UPROPERTY(VisibleAnywhere, Category="Occlusion Binder|Shadow")
    TArray<TObjectPtr<USkeletalMeshComponent>> SkeletalShadowProxies;

    // Parallel arrays — index matches NormalMeshes/OccludedMeshes per slot
    UPROPERTY(Transient)
    TArray<UMaterialInstanceDynamic*> NormalDynamicMaterials;

    UPROPERTY(Transient)
    TArray<UMaterialInstanceDynamic*> OccludedDynamicMaterials;

    // Tracks which MID indices belong to RT material meshes
    TArray<bool> NormalIsRTMaterial;
    TArray<bool> OccludedIsRTMaterial;

    UPROPERTY(Transient)
    TSet<TWeakObjectPtr<UObject>> ActiveOverlaps;

    float CurrentAlpha      = 1.f;
    bool  bShouldBeOccluded = false;
    bool  bForceOccluded    = false;
};