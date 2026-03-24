#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "ObstacleOcclusion/OcclusionTracer/OcclusionInterface.h"
#include "ObstacleOcclusion/MIDPool/OcclusionMIDSlot.h"
#include "OcclusionObstacleComp_Material.generated.h"

// FD
class UMeshComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UOcclusionObstacleComp_Material : public USceneComponent, public IOcclusionInterface
{
    GENERATED_BODY()

public:

    UOcclusionObstacleComp_Material();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category="Occlusion")
    void SetupOcclusionMeshes();

    UFUNCTION(BlueprintCallable, Category="Occlusion")
    void InitializeCollisionAndShadow();

    UFUNCTION(BlueprintCallable, Category="Occlusion")
    void SetFadeSpeed(float InFadeSpeed) { FadeSpeed = InFadeSpeed; }

    UFUNCTION(BlueprintCallable, Category="Occlusion")
    void SetShouldCastShadow(bool bCastShadow) { bCastShadowWhenOccluded = bCastShadow; }

    virtual void OnOcclusionEnter_Implementation(UObject* SourceTracer) override;
    virtual void OnOcclusionExit_Implementation(UObject* SourceTracer) override;
    virtual void ForceOcclude_Implementation(bool bForce) override;

protected:
    
    UPROPERTY(EditAnywhere, Category="Occlusion")
    float FadeSpeed = 6.f;

    UPROPERTY(EditAnywhere, Category="Occlusion")
    TEnumAsByte<ECollisionChannel> OcclusionTraceChannel = ECC_GameTraceChannel1;

    UPROPERTY(EditAnywhere, Category="Occlusion")
    TEnumAsByte<ECollisionChannel> MouseTraceChannel = ECC_Visibility;

    UPROPERTY(EditAnywhere, Category="Occlusion|Shadow")
    bool bCastShadowWhenOccluded = true;

    UPROPERTY(EditAnywhere, Category="Occlusion|Shadow")
    TObjectPtr<UMaterialInterface> ShadowProxyMaterial;

private:

    void DiscoverChildMeshes();
    void GenerateShadowProxyMeshes();
    void InitializeMaterials();
    void UpdateMaterialAlpha();
    void CleanupInvalidOverlaps();

    // ── Pool ──────────────────────────────────────────────────────────────
    void AcquireMIDs();
    void ReleaseMIDs();
    bool HasPooledMIDs() const;

    UPROPERTY(Transient)
    TSet<TWeakObjectPtr<UObject>> ActiveOverlaps;

    float CurrentAlpha      = 1.f;
    float CurrentForceAlpha = 0.f;
    bool  bShouldBeOccluded = false;
    bool  bForceOccluded    = false;

    // Unified slots — bPooled=false created at BeginPlay, bPooled=true checked out on occlusion enter
    UPROPERTY(Transient)
    TArray<FOcclusionMIDSlot> TargetSlots;

    UPROPERTY(VisibleAnywhere)
    TArray<TSoftObjectPtr<UMeshComponent>> TargetMeshes;

    UPROPERTY(VisibleAnywhere, Category="Occlusion|Shadow")
    TArray<TObjectPtr<UStaticMeshComponent>> StaticShadowProxies;

    UPROPERTY(VisibleAnywhere, Category="Occlusion|Shadow")
    TArray<TObjectPtr<USkeletalMeshComponent>> SkeletalShadowProxies;
};