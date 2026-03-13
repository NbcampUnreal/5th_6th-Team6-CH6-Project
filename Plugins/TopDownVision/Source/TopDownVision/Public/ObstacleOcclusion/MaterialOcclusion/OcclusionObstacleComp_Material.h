#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "ObstacleOcclusion/OcclusionInterface.h"
#include "OcclusionObstacleComp_Material.generated.h"

class UMeshComponent;
class UMaterialInstanceDynamic;

/**
 * Uses the same occlusion interface as the physical mesh swapper comp.
 * Does not swap meshes — drives material occlusion alpha on tagged meshes.
 * Supports both static and skeletal meshes via UMeshComponent.
 */

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

	virtual void OnOcclusionEnter_Implementation(UObject* SourceTracer) override;
	virtual void OnOcclusionExit_Implementation(UObject* SourceTracer) override;

private:

	void DiscoverChildMeshes();
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

private:
	
	UPROPERTY(Transient)
	TSet<TWeakObjectPtr<UObject>> ActiveOverlaps;

	float CurrentAlpha     = 0.f;
	bool bShouldBeOccluded = false;

	UPROPERTY(Transient)
	TArray<UMaterialInstanceDynamic*> DynamicMaterials;

	// UMeshComponent covers both static and skeletal meshes
	UPROPERTY(VisibleAnywhere)
	TArray<TSoftObjectPtr<UMeshComponent>> TargetMeshes;
};