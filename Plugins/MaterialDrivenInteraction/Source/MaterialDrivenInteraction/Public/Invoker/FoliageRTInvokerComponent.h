#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FoliageRTInvokerComponent.generated.h"

class URTPoolManager;
class UTextureRenderTarget2D;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UCLASS(ClassGroup = "Foliage RT", meta = (BlueprintSpawnableComponent))
class MATERIALDRIVENINTERACTION_API UFoliageRTInvokerComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UFoliageRTInvokerComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	// ── Brush assets ──────────────────────────────────────────────────────────

	/**
	 * Material drawn into ContinuousRT every tick.
	 * Receives: BrushCentreUV, BrushUVExtent, BrushRotation, Weight.
	 * Computes per-texel outward push direction from brush shape internally.
	 * C++ only passes position, rotation and weight.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Brush")
	TObjectPtr<UMaterialInterface> BrushMaterial_Continuous;

	/**
	 * Material drawn into ImpulseRT on new pixel entry.
	 * Receives: VelocityX, VelocityY, Weight, ImpactTime, BrushCentreUV, BrushUVExtent, ImpactTimeVec.
	 * Stamps GameTime into G — foliage reads TimeDelta = View.GameTime - G to drive FRT_DampedSine.
	 * Re-triggers automatically whenever the invoker moves out of and back into a texel.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Brush")
	TObjectPtr<UMaterialInterface> BrushMaterial_Impulse;

	/** World-space half-extents of the brush footprint. Also determines which cells are registered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Brush",
		meta = (ClampMin = 1.f))
	FVector2D BrushExtent = FVector2D(60.f, 60.f);

	/** Velocity magnitude that maps to 1.0 in the normalized range. Must match FRT_MaxVelocity in material. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Brush",
		meta = (ClampMin = 1.f))
	float MaxVelocity = 1200.f;

	/** Overall brush strength scalar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Brush",
		meta = (ClampMin = 0.f, ClampMax = 1.f))
	float BrushWeight = 1.f;

	// ── Runtime read-outs ─────────────────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, Category = "Foliage RT|State")
	TArray<int32> ActiveSlotIndices;

	UPROPERTY(BlueprintReadOnly, Category = "Foliage RT|State")
	FVector2D CurrentCellUV = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Foliage RT|State")
	FVector2D EncodedVelocity = FVector2D(0.5f, 0.5f);

	// ── Blueprint extension points ────────────────────────────────────────────

	UFUNCTION(BlueprintImplementableEvent, Category = "Foliage RT|Events")
	void OnContinuousPaint(UTextureRenderTarget2D* ContinuousRT,
	                       FVector2D               CellUV,
	                       FVector2D               BrushUVExtent);

	UFUNCTION(BlueprintImplementableEvent, Category = "Foliage RT|Events")
	void OnImpulseStamp(UTextureRenderTarget2D* ImpulseRT,
	                    FVector2D               CellUV,
	                    FVector2D               BrushUVExtent);

private:

	TArray<FIntPoint> GetOverlappingCells(const FVector& WorldLocation) const;
	void              UpdateCellRegistrations(const FVector& WorldLocation);

	FVector2D ComputeVelocity(float DeltaTime) const;
	float     EncodeVelocityAxis(float WorldVelAxis) const;
	FVector2D BrushExtentToUV() const;
	bool      HasMovedToNewPixel(FVector2D CellUV, const FIntPoint& PrevTexel) const;

	void DrawContinuousQuad(UTextureRenderTarget2D* RT,
	                        FVector2D CurrentUV,
	                        FVector2D UVExtent,
	                        float OwnerYaw);

	void DrawImpulseStamp(UTextureRenderTarget2D* RT,
	                      FVector2D CurrentUV, FVector2D UVExtent,
	                      float GameTime);

	// ── Cached references ─────────────────────────────────────────────────────

	UPROPERTY()
	TObjectPtr<URTPoolManager> PoolManager;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MID_Continuous;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MID_Impulse;

	// ── Per-frame state ───────────────────────────────────────────────────────

	FVector PrevWorldLocation = FVector::ZeroVector;

	TArray<FIntPoint>          CurrentCells;
	TMap<FIntPoint, FIntPoint> PrevImpulseTexelMap;

	bool bFirstTick = true;
};