// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "OcclusionInterface.h"
#include "OcclusionObstacleComponent.generated.h"

// Forward declares
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPrimitiveComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UOcclusionObstacleComponent : public USceneComponent, public IOcclusionInterface
{
    GENERATED_BODY()

public:
    UOcclusionObstacleComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /** Call this in editor or BeginPlay to register tagged meshes and set up materials */
    UFUNCTION(BlueprintCallable, Category="Occlusion Setup")
    void SetupOcclusionMeshes();

    // IOcclusionInterface
    virtual void OnOcclusionEnter_Implementation(UPrimitiveComponent* OccludingComponent) override;
    virtual void OnOcclusionExit_Implementation(UPrimitiveComponent* OccludingComponent) override;

private:

    void InitializeMaterials();
    void UpdateMaterialAlpha();
    void CleanupInvalidOverlaps();
    void DiscoverChildMeshes();

protected:

    // Tag used for normal (visible) meshes
    UPROPERTY(EditAnywhere, Category="Occlusion")
    FName NormalMeshTag = TEXT("OcclusionMesh");

    // Tag used for occluded (faded) meshes
    UPROPERTY(EditAnywhere, Category="Occlusion")
    FName OccludedMeshTag = TEXT("OccludedVisual");

    /** Fade interpolation speed */
    UPROPERTY(EditAnywhere, Category="Occlusion")
    float FadeSpeed = 6.f;

    /** Material scalar parameter driven by occlusion state */
    UPROPERTY(EditAnywhere, Category="Occlusion")
    FName AlphaParameterName = TEXT("OcclusionAlpha");

private:

    /** Active occluding components — tracked for multi-overlap safety */
    TSet<TWeakObjectPtr<UPrimitiveComponent>> ActiveOverlaps;

    float CurrentAlpha = 0.f;
    bool  bShouldBeOccluded = false;
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
