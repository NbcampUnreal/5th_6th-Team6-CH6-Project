#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ObstacleOcclusion/OcclusionTracer/OcclusionInterface.h"
#include "OcclusionBinder.generated.h"

struct FOcclusionMIDSlot;
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
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Binder")
    float FadeSpeed = 6.f;
    
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
    void UpdateOccludedMeshVisibility();

    // ── Pool ──────────────────────────────────────────────────────────────
    void AcquireMIDs();
    void ReleaseMIDs();
    bool HasPooledMIDs() const;

    // ── Mesh arrays ───────────────────────────────────────────────────────

    UPROPERTY(VisibleAnywhere, Category="Occlusion Binder")
    TArray<TSoftObjectPtr<UMeshComponent>> NormalMeshes;

    UPROPERTY(VisibleAnywhere, Category="Occlusion Binder")
    TArray<TSoftObjectPtr<UMeshComponent>> OccludedMeshes;

    UPROPERTY(VisibleAnywhere, Category="Occlusion Binder")
    TArray<TSoftObjectPtr<UMeshComponent>> RTMeshes;

    UPROPERTY(VisibleAnywhere, Category="Occlusion Binder|Shadow")
    TArray<TObjectPtr<UStaticMeshComponent>> StaticShadowProxies;

    UPROPERTY(VisibleAnywhere, Category="Occlusion Binder|Shadow")
    TArray<TObjectPtr<USkeletalMeshComponent>> SkeletalShadowProxies;

    // ── Unified slots (replaces 6 raw MID arrays) ─────────────────────────
    // bPooled=false → created at BeginPlay, kept forever
    // bPooled=true  → checked out on first occlusion enter, returned when fully visible

    UPROPERTY(Transient)
    TArray<FOcclusionMIDSlot> NormalSlots;

    UPROPERTY(Transient)
    TArray<FOcclusionMIDSlot> OccludedSlots;

    // RT meshes need both AlphaParameterName and ForceOccludeParameterName
    UPROPERTY(Transient)
    TArray<FOcclusionMIDSlot> RTSlots;

    UPROPERTY(Transient)
    TSet<TWeakObjectPtr<UObject>> ActiveOverlaps;

    float CurrentAlpha      = 1.f;
    float CurrentForceAlpha = 0.f;
    bool  bShouldBeOccluded = false;
    bool  bForceOccluded    = false;

    /*//one time only
    float CurrentRTMode = 0.f;   // 0 = Physical, 1 = RT
    float TargetRTMode  = 0.f;   // desired state*/
};