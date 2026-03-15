#pragma once

#include "CoreMinimal.h"
#include "OcclusionBrushTarget.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;

USTRUCT(BlueprintType)
struct FOcclusionBrushTarget
{
    GENERATED_BODY()

    // Shape component — bounds drive brush radius
    UPROPERTY()
    TWeakObjectPtr<UPrimitiveComponent> PrimitiveComp;

    UPROPERTY()
    float VisibleRadius = 100.f;

    // Scales brush intensity — drives visual effects in material
    UPROPERTY()
    float RevealAlpha = 1.f;

    // Per-target brush material — overrides painter default if set
    UPROPERTY()
    TObjectPtr<UMaterialInterface> BrushMaterial = nullptr;

    // Own MID — isolated params per target, no cross-contamination between brushes
    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> BrushMID = nullptr;

    bool IsValid() const { return PrimitiveComp.IsValid(); }

    bool IsMIDReady() const { return IsValid() && BrushMID != nullptr; }

    float GetVisibleRadius() const {return VisibleRadius; }

    // Called lazily by painter on first draw — Outer must be a valid UObject for GC
    void InitializeMID(UObject* Outer, UMaterialInterface* FallbackMaterial);
};