#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "OcclusionObstacleComponent.generated.h"

//ForwardDeclares
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPrimitiveComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UOcclusionObstacleComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UOcclusionObstacleComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;//reset
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	/** Meshes that detect overlap (have collision) */
	UPROPERTY(EditAnywhere, Category="Occlusion")
	TArray<UStaticMeshComponent*> NormalMeshes;

	/** Visual-only meshes */
	UPROPERTY(EditAnywhere, Category="Occlusion")
	TArray<UStaticMeshComponent*> OccludedMeshes;

	/** Fade speed */
	UPROPERTY(EditAnywhere, Category="Occlusion")
	float FadeSpeed = 6.f;

	/** Material parameter name */
	UPROPERTY(EditAnywhere, Category="Occlusion")
	FName AlphaParameterName = TEXT("OcclusionAlpha");

private:

	/** Active overlapping occluder sphere components */
	TSet<TWeakObjectPtr<UPrimitiveComponent>> ActiveOverlaps;

	/** Current fade alpha */
	float CurrentAlpha = 0.f;

	/** Target state */
	bool bShouldBeOccluded = false;

	/** Cached dynamic materials */
	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> DynamicMaterials;

	UFUNCTION()
	void OnMeshBeginOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnMeshEndOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	void InitializeCollision();
	void InitializeMaterials();
	void UpdateMaterialAlpha();
	void CleanupInvalidOverlaps();
};