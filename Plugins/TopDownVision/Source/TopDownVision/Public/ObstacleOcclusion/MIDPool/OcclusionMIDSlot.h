#pragma once

#include "CoreMinimal.h"
#include "OcclusionMIDSlot.generated.h"

class UMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

// Tracks a single checked-out MID — stores the mesh slot it occupies
// and the original material so it can be restored on return.
USTRUCT()
struct TOPDOWNVISION_API FOcclusionMIDSlot
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<UMeshComponent> Mesh;

	int32 SlotIndex = INDEX_NONE;

	// If true — MID came from pool, must be returned and original material restored.
	// If false — MID was created directly, lives until EndPlay, nothing to restore.
	bool bPooled = false;

	// The real asset that was on this slot before we applied the MID.
	// Always the parent asset — never another MID.
	UPROPERTY()
	TObjectPtr<UMaterialInterface> OriginalMaterial = nullptr;

	// Null when not checked out
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MID = nullptr;

	bool IsReady() const { return Mesh.IsValid() && MID != nullptr; }
};