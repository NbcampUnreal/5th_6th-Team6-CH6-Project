#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Data/RTBrushTypes.h"
#include "FoliageRTInvokerComponent.generated.h"

class URTPoolManager;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UCLASS(ClassGroup = "Foliage RT", meta = (BlueprintSpawnableComponent))
class MATERIALDRIVENINTERACTION_API UFoliageRTInvokerComponent : public USceneComponent
{
    GENERATED_BODY()

public:

    UFoliageRTInvokerComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
                               FActorComponentTickFunction* ThisTickFunction) override;

    // ── Brush assets ──────────────────────────────────────────────────────────

    /** Material drawn into ContinuousRT every draw cycle. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Brush")
    TObjectPtr<UMaterialInterface> BrushMaterial_Continuous;

    /** Material drawn into ImpulseRT every draw cycle via SE_BLEND_MAX.
     *  Writes velocity RG + frac(GameTime/Period) to A every tick.
     *  A freezes on exit — consumer reads age = frac(CurrentTime - A). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Brush")
    TObjectPtr<UMaterialInterface> BrushMaterial_Impulse;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Brush",
        meta = (ClampMin = 1.f))
    float BrushRadius = 60.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Brush",
        meta = (ClampMin = 1.f))
    float MaxVelocity = 1200.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Brush",
        meta = (ClampMin = 0.f, ClampMax = 1.f))
    float BrushWeight = 1.f;

    /** Period for frac(GameTime/Period) timestamp encoding.
     *  Consumer spring decay must complete within this window. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Brush",
        meta = (ClampMin = 1.f))
    float TimestampPeriod = 60.f;

    // ── Frame data ────────────────────────────────────────────────────────────

    FRTInvokerFrameData GetFrameData(int32 SlotIndex, FVector2D CellOriginWS, float CellSize) const;

    // ── Runtime read-outs ─────────────────────────────────────────────────────

    UPROPERTY(BlueprintReadOnly, Category = "Foliage RT|State")
    FVector2D EncodedVelocity = FVector2D(0.5f, 0.5f);

    // ── Blueprint extension points ────────────────────────────────────────────

    UFUNCTION(BlueprintImplementableEvent, Category = "Foliage RT|Events")
    void OnContinuousPaint(FVector2D CellUV, float UVRadius);

    UFUNCTION(BlueprintImplementableEvent, Category = "Foliage RT|Events")
    void OnImpulsePaint(FVector2D CellUV, float UVRadius);

private:

    FVector2D ComputeVelocity(float DeltaTime) const;
    float     EncodeVelocityAxis(float WorldVelAxis) const;

    // PrevImpulseTexelMap removed — impulse drawn every tick, no stamp gating
    // HasMovedToNewPixel removed

    UPROPERTY()
    TObjectPtr<URTPoolManager> PoolManager;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> MID_Continuous;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> MID_Impulse;

    FVector PrevWorldLocation = FVector::ZeroVector;
    bool    bFirstTick        = true;
};