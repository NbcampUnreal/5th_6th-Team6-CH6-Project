#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ObstacleOcclusion/OcclusionTracer/OcclusionInterface.h"
#include "OcclusionBinder.generated.h"

// FD
class UMeshComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UMaterialInstanceDynamic;
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

    // Discovers meshes from all bound actors and sets their collision —
    // call once in editor after populating BoundActors
    UFUNCTION(CallInEditor, BlueprintCallable, Category="Occlusion Binder")
    void SetupBoundActors();

    // ── IOcclusionInterface ───────────────────────────────────────────────

    virtual void OnOcclusionEnter_Implementation(UObject* SourceTracer) override;
    virtual void OnOcclusionExit_Implementation(UObject* SourceTracer) override;

    // ── Config ────────────────────────────────────────────────────────────

    // Actors to bind — populate in editor by dragging world actors here
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Binder")
    TArray<TObjectPtr<AActor>> BoundActors;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Binder")
    float FadeSpeed = 6.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Binder")
    FName AlphaParameterName = TEXT("OcclusionAlpha");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Binder")
    TEnumAsByte<ECollisionChannel> OcclusionTraceChannel = ECC_GameTraceChannel1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Occlusion Binder")
    TEnumAsByte<ECollisionChannel> MouseTraceChannel = ECC_Visibility;

private:

    void DiscoverAndRegisterMeshes();
    void InitializeMaterials();
    void UpdateMaterialAlpha();
    void CleanupInvalidOverlaps();

    // Runtime MIDs — created from bound actors' mesh materials at BeginPlay
    UPROPERTY(Transient)
    TArray<UMaterialInstanceDynamic*> DynamicMaterials;

    // Cached mesh components from all bound actors — used for MID creation
    UPROPERTY(Transient)
    TArray<TWeakObjectPtr<UMeshComponent>> BoundMeshes;

    UPROPERTY(Transient)
    TSet<TWeakObjectPtr<UObject>> ActiveOverlaps;

    float CurrentAlpha      = 0.f;
    bool  bShouldBeOccluded = false;
};