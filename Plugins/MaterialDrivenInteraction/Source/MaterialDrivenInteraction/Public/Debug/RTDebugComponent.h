#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RTDebugComponent.generated.h"

class URTPoolManager;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UStaticMesh;
class UMaterial;

UCLASS(ClassGroup = "Foliage RT", meta = (BlueprintSpawnableComponent))
class MATERIALDRIVENINTERACTION_API URTDebugComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	URTDebugComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Debug")
	bool bShowDebugBoxes = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Debug")
	bool bShowDebugPlanes = true;

	/** true = show ImpulseRT, false = show ContinuousRT. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Debug")
	bool bShowImpulseRT = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Debug")
	float PlaneHeight = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Debug")
	FColor ActiveBoxColor = FColor(100, 220, 255);

	/** Unlit material with a Texture2D parameter named "DebugTex". Emissive = TextureParam. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Debug")
	TObjectPtr<UMaterialInterface> DebugPlaneMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Debug")
	TObjectPtr<UStaticMesh> PlaneMesh;

	UFUNCTION(BlueprintCallable, Category = "Foliage RT|Debug")
	void RefreshDebugPlanes();

private:

	void DrawBoxes();
	void UpdatePlanes();
	void ResizePlanePool(int32 Count);
	void UpdatePlane(int32 PlaneIndex, int32 SlotIndex);
	void HidePlane(int32 PlaneIndex);

	UPROPERTY()
	TObjectPtr<URTPoolManager> PoolManager;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> DebugPlanes;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> PlaneMIDs;

	static const FName PN_DebugTex;
};