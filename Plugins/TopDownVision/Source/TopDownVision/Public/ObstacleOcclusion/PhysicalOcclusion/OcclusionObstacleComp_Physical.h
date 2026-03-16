#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "ObstacleOcclusion/OcclusionTracer/OcclusionInterface.h"
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

    UFUNCTION(BlueprintCallable, Category="Occlusion Setup")
    void SetupOcclusionMeshes();

    UFUNCTION(BlueprintCallable, Category="Occlusion Setup")
    void InitializeCollisionAndShadow();

    UFUNCTION(BlueprintCallable, Category="Occlusion")
    void SetFadeSpeed(float InFadeSpeed) { FadeSpeed = InFadeSpeed; }

    UFUNCTION(BlueprintCallable, Category="Occlusion")
    void SetShouldCastShadow(bool bCastShadow) { bCastShadowWhenOccluded = bCastShadow; }

    virtual void OnOcclusionEnter_Implementation(UObject* SourceTracer) override;
    virtual void OnOcclusionExit_Implementation(UObject* SourceTracer) override;

private:

    void GenerateShadowProxyMeshes();  // delegates to OcclusionMeshUtil
    void InitializeMaterials();
    void UpdateMaterialAlpha();
    void CleanupInvalidOverlaps();
    void DiscoverChildMeshes();

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

    UPROPERTY(EditAnywhere, Category="Occlusion|Shadow")
    bool bCastShadowWhenOccluded = true;
    
    UPROPERTY(EditAnywhere, Category="Occlusion|Shadow")
    TObjectPtr<UMaterialInterface> ShadowProxyMaterial;

private:

    UPROPERTY(Transient)
    TSet<TWeakObjectPtr<UObject>> ActiveOverlaps;

    float CurrentAlpha        = 0.f;
    bool  bShouldBeOccluded   = false;
    bool  bLastOcclusionState = false;

    UPROPERTY(Transient)
    TArray<UMaterialInstanceDynamic*> NormalStaticMIDs;

    UPROPERTY(Transient)
    TArray<UMaterialInstanceDynamic*> NormalSkeletalMIDs;

    UPROPERTY(Transient)
    TArray<UMaterialInstanceDynamic*> OccludedStaticMIDs;

    UPROPERTY(Transient)
    TArray<UMaterialInstanceDynamic*> OccludedSkeletalMIDs;

    UPROPERTY(VisibleAnywhere)
    TArray<TSoftObjectPtr<UMeshComponent>> NormalMeshes;

    UPROPERTY(VisibleAnywhere)
    TArray<TSoftObjectPtr<UMeshComponent>> OccludedMeshes;

    UPROPERTY(VisibleAnywhere, Category="Occlusion|Shadow")
    TArray<TObjectPtr<UStaticMeshComponent>> StaticShadowProxies;

    UPROPERTY(VisibleAnywhere, Category="Occlusion|Shadow")
    TArray<TObjectPtr<USkeletalMeshComponent>> SkeletalShadowProxies;
};