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

	UFUNCTION(BlueprintCallable, Category="Occlusion Setup")
	void SetupOcclusionMeshes();// this is for registering the mesh in the editor
	
protected:
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

private:
	//internal helpers
	void InitializeCollision();
	void InitializeMaterials();
	void UpdateMaterialAlpha();
	void CleanupInvalidOverlaps();
	void DiscoverChildMeshes();

	
protected:
	/*/** Meshes that detect overlap (have collision) #1#
	UPROPERTY(EditAnywhere, Category="Occlusion")
	TArray<UStaticMeshComponent*> NormalMeshes;
	/** Visual-only meshes #1#
	UPROPERTY(EditAnywhere, Category="Occlusion")
	TArray<UStaticMeshComponent*> OccludedMeshes;*/

	
	//!!!!! Now use the Tag for the mesh so that the mesh can be placed freely in the editor-> register will be happened in begin play
	// Tag used for collision meshes
	UPROPERTY(EditAnywhere, Category="Occlusion")
	FName NormalMeshTag = TEXT("OcclusionMesh");

	// Tag used for visual-only meshes
	UPROPERTY(EditAnywhere, Category="Occlusion")
	FName OccludedMeshTag = TEXT("OccludedVisual");

	/** Custom Trace Channel for occlusion probe detection */
	UPROPERTY(EditAnywhere, Category="Occlusion")
	TEnumAsByte<ECollisionChannel> OcclusionProbeChannel = ECC_GameTraceChannel1;

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
	TArray<UMaterialInstanceDynamic*> NormalDynamicMaterials;
	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> OccludedDynamicMaterials;

	//registered meshes
	UPROPERTY(VisibleAnywhere, Instanced)
	TArray<UStaticMeshComponent*> NormalMeshes;
	UPROPERTY(VisibleAnywhere, Instanced)
	TArray<UStaticMeshComponent*> OccludedMeshes;

	bool bLastOcclusionState = false;


};