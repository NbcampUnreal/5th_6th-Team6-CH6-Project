#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ObstacleOcclusion/MaterialOcclusion/OcclusionBrushTarget.h"
#include "OcclusionSubsystem.generated.h"

class UPrimitiveComponent;

TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(OcclusionSubsystem, Log, All);

UCLASS()
class TOPDOWNVISION_API UOcclusionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	// ── Target registration — called by actor BP via OnTracerActivated/Deactivated ──

	UFUNCTION(BlueprintCallable, Category="OcclusionSubsystem")
	void RegisterTarget(UPrimitiveComponent* PrimComp, float RadiusPadding = 30.f);

	UFUNCTION(BlueprintCallable, Category="OcclusionSubsystem")
	void UnregisterTarget(UPrimitiveComponent* PrimComp);

	// ── Read by MainOcclusionPainter each draw ────────────────────────────────────

	const TArray<FOcclusionBrushTarget>& GetTargets() const { return Targets; }

private:

	TArray<FOcclusionBrushTarget> Targets;
};