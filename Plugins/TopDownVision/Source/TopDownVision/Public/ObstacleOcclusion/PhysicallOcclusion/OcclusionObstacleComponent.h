#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "OcclusionInterface.h"
#include "OcclusionObstacleComponent.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UOcclusionObstacleComponent : public USceneComponent, public IOcclusionInterface
{
    GENERATED_BODY()

public:
    UOcclusionObstacleComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category="Occlusion Setup")
    void SetupOcclusionMeshes();
    
    void InitializeCollision();

    virtual void OnOcclusionEnter_Implementation(UObject* SourceTracer) override;
    virtual void OnOcclusionExit_Implementation(UObject* SourceTracer) override;

private:

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
private:

    TSet<TWeakObjectPtr<UObject>> ActiveOverlaps;

    float CurrentAlpha        = 0.f;
    bool  bShouldBeOccluded   = false;
    bool  bLastOcclusionState = false;

    UPROPERTY(Transient)
    TArray<UMaterialInstanceDynamic*> NormalDynamicMaterials;

    UPROPERTY(Transient)
    TArray<UMaterialInstanceDynamic*> OccludedDynamicMaterials;

    UPROPERTY(VisibleAnywhere)
    TArray<TSoftObjectPtr<UStaticMeshComponent>> NormalMeshes;

    UPROPERTY(VisibleAnywhere)
    TArray<TSoftObjectPtr<UStaticMeshComponent>> OccludedMeshes;
};