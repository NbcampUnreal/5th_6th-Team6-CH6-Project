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

    UFUNCTION(CallInEditor, BlueprintCallable, Category="Occlusion Binder")
    void SetupBoundActors();

    virtual void OnOcclusionEnter_Implementation(UObject* SourceTracer) override;
    virtual void OnOcclusionExit_Implementation(UObject* SourceTracer) override;
    virtual void ForceOcclude_Implementation(bool bForce) override;

    // ── Config ────────────────────────────────────────────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Binder")
    TArray<TObjectPtr<AActor>> BoundActors;

    // Fades OUT when occluded — same as physical comp NormalMeshTag
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Binder")
    FName NormalMeshTag = TEXT("OcclusionMesh");

    // Fades IN when occluded — same as physical comp OccludedMeshTag
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Binder")
    FName OccludedMeshTag = TEXT("OccludedVisual");

    // RT material mesh — fades out AND receives ForceOccluded param
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Binder")
    FName RTMaterialTag = TEXT("RTOcclusionMesh");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Binder")
    float FadeSpeed = 6.f;

    // Shared alpha param name — same as both comps
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Binder")
    FName AlphaParameterName = TEXT("OcclusionAlpha");

    // Force occlude param — only sent to RT material meshes
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Binder")
    FName ForceOccludeParameterName = TEXT("FullOcclusionAlpha");

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

    //void UpdateMouseTraceCollision(bool bOccluded);// now the mouse collision will be only ignored when it is occluded

    // ── Mesh arrays — same pattern as physical comp ───────────────────────

    // NormalMeshTag + RTMaterialTag meshes — fade OUT when occluded
    UPROPERTY(VisibleAnywhere, Category="Occlusion Binder")
    TArray<TSoftObjectPtr<UMeshComponent>> NormalMeshes;

    // OccludedMeshTag meshes — fade IN when occluded
    UPROPERTY(VisibleAnywhere, Category="Occlusion Binder")
    TArray<TSoftObjectPtr<UMeshComponent>> OccludedMeshes;

    // RTMaterialTag meshes — subset of NormalMeshes, also receive ForceOccluded param
    UPROPERTY(VisibleAnywhere, Category="Occlusion Binder")
    TArray<TSoftObjectPtr<UMeshComponent>> RTMeshes;

    UPROPERTY(VisibleAnywhere, Category="Occlusion Binder|Shadow")
    TArray<TObjectPtr<UStaticMeshComponent>> StaticShadowProxies;

    UPROPERTY(VisibleAnywhere, Category="Occlusion Binder|Shadow")
    TArray<TObjectPtr<USkeletalMeshComponent>> SkeletalShadowProxies;

    // ── MIDs — same pattern as physical comp ─────────────────────────────

    UPROPERTY(Transient)
    TArray<UMaterialInstanceDynamic*> NormalStaticMIDs;

    UPROPERTY(Transient)
    TArray<UMaterialInstanceDynamic*> NormalSkeletalMIDs;

    UPROPERTY(Transient)
    TArray<UMaterialInstanceDynamic*> OccludedStaticMIDs;

    UPROPERTY(Transient)
    TArray<UMaterialInstanceDynamic*> OccludedSkeletalMIDs;

    // RT material MIDs — also receive ForceOccluded param
    UPROPERTY(Transient)
    TArray<UMaterialInstanceDynamic*> RTStaticMIDs;

    UPROPERTY(Transient)
    TArray<UMaterialInstanceDynamic*> RTSkeletalMIDs;

    UPROPERTY(Transient)
    TSet<TWeakObjectPtr<UObject>> ActiveOverlaps;

    // Same convention as physical comp — 1 = visible, 0 = occluded
    float CurrentAlpha      = 1.f;
    // Same convention as material comp — 0 = not forced, 1 = forced
    float CurrentForceAlpha = 0.f;

    bool bShouldBeOccluded = false;
    bool bForceOccluded    = false;
};