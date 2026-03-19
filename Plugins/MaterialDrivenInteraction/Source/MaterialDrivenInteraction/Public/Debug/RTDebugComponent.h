#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RTDebugComponent.generated.h"

class URTPoolManager;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UStaticMesh;
class UMaterial;

// ──────────────────────────────────────────────────────────────────────────────
// URTDebugComponent
//
// Attach to any actor alongside URTMPCUpdater.
// Each tick:
//   • Draws a debug box outline for every active pool slot cell.
//   • Maintains a set of plane meshes in the world, one per slot, each
//     displaying the live ImpulseRT or ContinuousRT texture.
//
// Toggle RT type with bShowImpulseRT.
// Toggle visibility with bShowDebugBoxes / bShowDebugPlanes.
// ──────────────────────────────────────────────────────────────────────────────
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

	// ── Config ────────────────────────────────────────────────────────────────

	/** Draw a wire box in the world for each active cell. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Debug")
	bool bShowDebugBoxes = true;

	/** Spawn flat plane meshes showing the live RT texture per slot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Debug")
	bool bShowDebugPlanes = true;

	/** true = show ImpulseRT,  false = show ContinuousRT. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Debug")
	bool bShowImpulseRT = true;

	/** Height above ground to float the debug planes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Debug")
	float PlaneHeight = 300.f;

	/** Color for active slot boxes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Debug")
	FColor ActiveBoxColor = FColor(100, 220, 255);

	/**
	 * A simple unlit material with a single Texture2D parameter named "DebugTex".
	 * Used as the parent for the plane MIDs.
	 * Create in editor: unlit surface, Emissive = TextureParam "DebugTex".
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Debug")
	TObjectPtr<UMaterialInterface> DebugPlaneMaterial;

	/**
	 * The plane static mesh used for RT preview (default engine plane is fine:
	 * /Engine/BasicShapes/Plane.Plane).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Debug")
	TObjectPtr<UStaticMesh> PlaneMesh;

	// ── Blueprint controls ────────────────────────────────────────────────────

	/** Rebuild all debug planes immediately (call after changing bShowImpulseRT). */
	UFUNCTION(BlueprintCallable, Category = "Foliage RT|Debug")
	void RefreshDebugPlanes();

private:

	void DrawBoxes();
	void UpdatePlanes();

	/** Ensure we have exactly Count plane components, creating or destroying as needed. */
	void ResizePlanePool(int32 Count);

	/** Position, scale and update the MID for a single plane. */
	void UpdatePlane(int32 PlaneIndex, int32 SlotIndex);

	/** Hide a plane (slot is inactive). */
	void HidePlane(int32 PlaneIndex);

	UPROPERTY()
	TObjectPtr<URTPoolManager> PoolManager;

	/** One plane mesh component per pool slot. */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> DebugPlanes;

	/** One MID per plane — receives the RT texture each tick. */
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> PlaneMIDs;

	static const FName PN_DebugTex;
};
