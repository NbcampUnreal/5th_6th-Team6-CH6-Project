#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "ObstacleOcclusion/OcclusionTracer/OcclusionInterface.h"
#include "OcclusionObstacleComp_Material.generated.h"

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
    
    
private:

    void DiscoverChildMeshes();
    void GenerateShadowProxyMeshes();  // delegates to OcclusionMeshUtil
    void InitializeMaterials();
    void UpdateMaterialAlpha();
    void CleanupInvalidOverlaps();

protected:

    UPROPERTY(EditAnywhere, Category="Occlusion")
    FName MeshTag = TEXT("OcclusionMesh");

    UPROPERTY(EditAnywhere, Category="Occlusion")
    float FadeSpeed = 6.f;

    UPROPERTY(EditAnywhere, Category="Occlusion")
    FName AlphaParameterName = TEXT("OcclusionAlpha");

    UPROPERTY(EditAnywhere, Category="Occlusion")
    FName ForceOccludeParameterName = TEXT("FullOcclusionAlpha");// this is for making the mesh fade not only the brush area

    UPROPERTY(EditAnywhere, Category="Occlusion")
    TEnumAsByte<ECollisionChannel> OcclusionTraceChannel = ECC_GameTraceChannel1;

    UPROPERTY(EditAnywhere, Category="Occlusion")
    TEnumAsByte<ECollisionChannel> MouseTraceChannel = ECC_Visibility;

    //Decide making shadow proxy mesh or not
    UPROPERTY(EditAnywhere, Category="Occlusion|Shadow")
    bool bCastShadowWhenOccluded = true;
    
    // Fully opaque proxy material — WPO driven by MPC, shadows only
    UPROPERTY(EditAnywhere, Category="Occlusion|Shadow")
    TObjectPtr<UMaterialInterface> ShadowProxyMaterial;

private:

    UPROPERTY(Transient)
    TSet<TWeakObjectPtr<UObject>> ActiveOverlaps;

    float CurrentAlpha       =   1.f; // 1= visible 0 occluded
    float CurrentForceAlpha  =   0.f; // 0 not fully hidden 1 all hidden
    bool  bShouldBeOccluded  = false;

    bool bForceOccluded      = false;
    //                       =      ;
    // fuck

    UPROPERTY(Transient)
    TArray<UMaterialInstanceDynamic*> StaticMIDs;

    UPROPERTY(Transient)
    TArray<UMaterialInstanceDynamic*> SkeletalMIDs;

    UPROPERTY(VisibleAnywhere)
    TArray<TSoftObjectPtr<UMeshComponent>> TargetMeshes;

    UPROPERTY(VisibleAnywhere, Category="Occlusion|Shadow")
    TArray<TObjectPtr<UStaticMeshComponent>> StaticShadowProxies;

    UPROPERTY(VisibleAnywhere, Category="Occlusion|Shadow")
    TArray<TObjectPtr<USkeletalMeshComponent>> SkeletalShadowProxies;
};