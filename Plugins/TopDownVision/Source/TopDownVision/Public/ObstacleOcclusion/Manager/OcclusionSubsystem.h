#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ObstacleOcclusion/MaterialOcclusion/OcclusionBrushTarget.h"
#include "OcclusionSubsystem.generated.h"

class UPrimitiveComponent;
class UMaterialInterface;

TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(OcclusionSubsystem, Log, All);

UCLASS()
class TOPDOWNVISION_API UOcclusionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	// ── Target registration — called by actor BP via OnTracerActivated/Deactivated ──

	// BrushMat — per-target brush material, overrides painter default if set
	UFUNCTION(BlueprintCallable, Category="OcclusionSubsystem")
	int32  RegisterTarget(UPrimitiveComponent* PrimComp, UMaterialInterface* BrushMat, float VisionHalfRadius);

	UFUNCTION(BlueprintCallable, Category="OcclusionSubsystem")
	void UnregisterTarget(UPrimitiveComponent* PrimComp);

	UFUNCTION(BlueprintCallable, Category="OcclusionSubsystem")
	void UpdateTargetByIndex(int32 Index, float NewRevealAlpha, float VisionHalfRadius);
	
	// ── Read by MainOcclusionPainter each draw ────────────────────────────────────

	const TArray<FOcclusionBrushTarget>& GetTargets() const { return Targets; }

	// Mutable — painter initializes MID lazily on first draw
	TArray<FOcclusionBrushTarget>& GetTargetsMutable() { return Targets; }

private:
	UPROPERTY(Transient)
	TArray<FOcclusionBrushTarget> Targets;
};