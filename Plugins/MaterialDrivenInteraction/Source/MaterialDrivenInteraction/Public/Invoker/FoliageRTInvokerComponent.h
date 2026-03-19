#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FoliageRTInvokerComponent.generated.h"

class URTPoolManager;
class UTextureRenderTarget2D;
class UMaterialInterface;
class UMaterialInstanceDynamic;

// ──────────────────────────────────────────────────────────────────────────────
// UFoliageRTInvokerComponent
//
// Attach to any Pawn or NPC.  Each tick it:
//   1. Resolves / registers the pool slot for the actor's current cell.
//   2. Paints ContinuousRT with a live-velocity smear quad every frame.
//   3. Stamps ImpulseRT only when the brush footprint has moved into a new
//      pixel — encoding GameTime in G and the velocity snapshot in R/B.
//   4. Drives URTPoolManager::Tick so slots reclaim without a separate actor.
//
// Cell boundary crossing is handled automatically — the component unregisters
// from the old cell and registers in the new one in a single frame.
// ──────────────────────────────────────────────────────────────────────────────
UCLASS(ClassGroup = "Foliage RT", meta = (BlueprintSpawnableComponent))
class MATERIALDRIVENINTERACTION_API UFoliageRTInvokerComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UFoliageRTInvokerComponent();

	// ── UActorComponent ───────────────────────────────────────────────────────

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	// ── Brush assets ──────────────────────────────────────────────────────────

	/**
	 * Material drawn into ContinuousRT every tick while the invoker is present.
	 * Receives scalar parameters each frame:
	 *   "VelocityX"  — X velocity remapped to [0,1]  (0.5 = stationary)
	 *   "VelocityY"  — Y velocity remapped to [0,1]
	 *   "Weight"     — brush strength [0,1]
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Brush")
	TObjectPtr<UMaterialInterface> BrushMaterial_Continuous;

	/**
	 * Material drawn into ImpulseRT when the brush enters a new pixel.
	 * Receives scalar parameters at stamp time:
	 *   "VelocityX"  — X velocity at impact remapped to [0,1]
	 *   "VelocityY"  — Y velocity at impact
	 *   "ImpactTime" — raw GameTime float  (foliage reads: TimeDelta = Now - G)
	 *   "Weight"     — brush strength [0,1]
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Brush")
	TObjectPtr<UMaterialInterface> BrushMaterial_Impulse;

	/**
	 * World-space half-extents of the brush footprint.
	 * Capsule character  → (CapsuleRadius, CapsuleRadius)
	 * Vehicle / long box → (HalfLength, HalfWidth)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Brush",
		meta = (ClampMin = 1.f))
	FVector2D BrushExtent = FVector2D(60.f, 60.f);

	/**
	 * World-space velocity magnitude that maps to 1.0 in the normalized range.
	 * Set to slightly above your actor's top speed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Brush",
		meta = (ClampMin = 1.f))
	float MaxVelocity = 1200.f;

	/** Overall brush strength scalar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|Brush",
		meta = (ClampMin = 0.f, ClampMax = 1.f))
	float BrushWeight = 1.f;

	// ── Runtime read-outs ─────────────────────────────────────────────────────

	/** Pool slot currently serving this invoker. INDEX_NONE if pool is full. */
	UPROPERTY(BlueprintReadOnly, Category = "Foliage RT|State")
	int32 ActiveSlotIndex = INDEX_NONE;

	/** UV of the brush centre within the current cell [0,1]. */
	UPROPERTY(BlueprintReadOnly, Category = "Foliage RT|State")
	FVector2D CurrentCellUV = FVector2D::ZeroVector;

	/**
	 * Velocity encoded for the RT: each axis remapped
	 * [−MaxVelocity..+MaxVelocity] → [0..1].  0.5 on both axes = stationary.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Foliage RT|State")
	FVector2D EncodedVelocity = FVector2D(0.5f, 0.5f);

	// ── Blueprint extension points ────────────────────────────────────────────

	/**
	 * Fired after the continuous paint each tick.
	 * Use to draw secondary brushes (limbs, wheels, etc.) into the same RT.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Foliage RT|Events")
	void OnContinuousPaint(UTextureRenderTarget2D* ContinuousRT,
	                       FVector2D               CellUV,
	                       FVector2D               BrushUVExtent);

	/**
	 * Fired after every ImpulseRT stamp.
	 * Use to trigger audio, VFX, or additional impulse layers.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Foliage RT|Events")
	void OnImpulseStamp(UTextureRenderTarget2D* ImpulseRT,
	                    FVector2D               CellUV,
	                    FVector2D               BrushUVExtent);

private:

	// ── Internal helpers ──────────────────────────────────────────────────────

	/** Compute world-space XY velocity from this frame's position delta. */
	FVector2D ComputeVelocity(float DeltaTime) const;

	/**
	 * Encode a signed velocity component into the [0,1] range stored in the RT.
	 * 0.5 = zero.  Material decode: WorldVel = (RTValue - 0.5) * 2.0 * MaxVelocity
	 */
	float EncodeVelocityAxis(float WorldVelAxis) const;

	/** Convert world-space brush half-extents to UV-space half-extents. */
	FVector2D BrushExtentToUV() const;

	/**
	 * Returns true when the brush centre has crossed into a new ImpulseRT texel.
	 * True on the very first tick (PrevImpulseTexel == (-1,-1)).
	 */
	bool HasMovedToNewPixel(FVector2D CellUV) const;

	/** Draw a smear quad spanning PrevUV → CurrentUV into ContinuousRT. */
	void DrawContinuousQuad(UTextureRenderTarget2D* RT,
	                        FVector2D PrevUV, FVector2D CurrentUV,
	                        FVector2D UVExtent);

	/** Stamp a single oriented quad at CurrentUV into ImpulseRT. */
	void DrawImpulseStamp(UTextureRenderTarget2D* RT,
	                      FVector2D CurrentUV, FVector2D UVExtent,
	                      float GameTime);

	/**
	 * Detect and handle cell-boundary crossings.
	 * Unregisters from the previous cell, registers in the new one.
	 */
	void HandleCellTransition(const FVector& WorldLocation);

	// ── Cached references ─────────────────────────────────────────────────────

	UPROPERTY()
	TObjectPtr<URTPoolManager> PoolManager;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MID_Continuous;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MID_Impulse;

	// ── Per-frame state ───────────────────────────────────────────────────────

	// World position at the end of the previous tick — used for velocity and smear.
	FVector   PrevWorldLocation  = FVector::ZeroVector;

	// Cell UV at the end of the previous tick — used as the smear quad start point.
	FVector2D PrevCellUV         = FVector2D(-1.f, -1.f);  // -1 forces first-frame stamp

	// Last ImpulseRT texel the brush occupied — changes trigger a new stamp.
	FIntPoint PrevImpulseTexel   = FIntPoint(-1, -1);

	// Cell the invoker was in last tick — changes trigger HandleCellTransition.
	FIntPoint CurrentCell        = FIntPoint(INT32_MIN, INT32_MIN);

	// True once BeginPlay has successfully registered with the pool.
	bool bRegistered = false;

	// Skips the very first tick so velocity and UV are seeded correctly before
	// any stamp or draw call fires. Prevents a G=0 stamp on spawn.
	bool bFirstTick = true;
};
