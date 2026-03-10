#pragma once

#include "CoreMinimal.h"
#include "OcclusionBrushTarget.generated.h"

USTRUCT(BlueprintType)
struct FOcclusionBrushTarget
{
    GENERATED_BODY()

    UPROPERTY()
    TWeakObjectPtr<UPrimitiveComponent> PrimitiveComp;

    UPROPERTY()
    float RadiusPadding = 30.f;

    UPROPERTY()
    float RevealAlpha = 1.f;

    bool IsValid() const { return PrimitiveComp.IsValid(); }

    float GetRadius() const
    {
        return PrimitiveComp.IsValid()
            ? PrimitiveComp->Bounds.SphereRadius + RadiusPadding
            : 0.f;
    }
};