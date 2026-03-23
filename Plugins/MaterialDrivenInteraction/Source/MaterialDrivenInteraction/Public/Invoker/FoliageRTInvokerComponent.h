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

	/** Material drawn into ContinuousRT every draw cycle.
	 *  Receives: BrushCentreUV, BrushUVExtent, BrushRotation, Weight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Brush")
	TObjectPtr<UMaterialInterface> BrushMaterial_Continuous;

	/** Material drawn into ImpulseRT on new texel entry.
	 *  Receives: VelocityX, VelocityY, ImpactTime, BrushCentreUV, BrushUVExtent. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Brush")
	TObjectPtr<UMaterialInterface> BrushMaterial_Impulse;

	/** World-space radius of the brush footprint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Brush",
		meta = (ClampMin = 1.f))
	float BrushRadius = 60.f;

	/** Velocity magnitude that maps to 1.0 in the normalized range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Brush",
		meta = (ClampMin = 1.f))
	float MaxVelocity = 1200.f;

	/** Overall brush strength scalar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Brush",
		meta = (ClampMin = 0.f, ClampMax = 1.f))
	float BrushWeight = 1.f;

	// ── Frame data — pulled by URTDrawManager each draw cycle ─────────────────

	// Called by URTDrawManager during draw cycle.
	// Builds and returns current frame data — sets MID parameters internally.
	FRTInvokerFrameData GetFrameData(int32 SlotIndex, FVector2D CellOriginWS, float CellSize) const;

	// ── Runtime read-outs ─────────────────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, Category = "Foliage RT|State")
	FVector2D EncodedVelocity = FVector2D(0.5f, 0.5f);

	// ── Blueprint extension points ────────────────────────────────────────────

	UFUNCTION(BlueprintImplementableEvent, Category = "Foliage RT|Events")
	void OnContinuousPaint(FVector2D CellUV, float UVRadius);

	UFUNCTION(BlueprintImplementableEvent, Category = "Foliage RT|Events")
	void OnImpulseStamp(FVector2D CellUV, float UVRadius);

private:

	FVector2D ComputeVelocity(float DeltaTime) const;
	float     EncodeVelocityAxis(float WorldVelAxis) const;
	bool      HasMovedToNewPixel(FVector2D CellUV, const FIntPoint& PrevTexel) const;

	// ── Cached references ─────────────────────────────────────────────────────

	UPROPERTY()
	TObjectPtr<URTPoolManager> PoolManager;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MID_Continuous;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MID_Impulse;

	// ── Per-frame state ───────────────────────────────────────────────────────

	FVector PrevWorldLocation = FVector::ZeroVector;

	// Per-cell texel tracking for impulse stamp gating.
	// Mutable so GetFrameData (const) can update it.
	mutable TMap<FIntPoint, FIntPoint> PrevImpulseTexelMap;

	bool bFirstTick = true;
};