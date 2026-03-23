#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RTDrawManager.generated.h"

class URTPoolManager;
class UFoliageRTInvokerComponent;
class UTextureRenderTarget2D;
class UCanvas;

UCLASS()
class MATERIALDRIVENINTERACTION_API URTDrawManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	// Called by URTTickManager at configured interval.
	// Full draw cycle:
	//   1. Pool evaluates registered invokers and assigns slots by priority
	//   2. Clear pending ImpulseRTs (flagged by pool manager on slot reassignment)
	//   3. Clear ContinuousRTs for occupied slots
	//   4. Draw all assigned invokers into their RTs
	//   5. Reclaim expired slots

	UFUNCTION(BlueprintCallable, Category = "RTDrawManager")
	void Update(float DeltaTime);

private:

	// Step 1 — pool evaluation result for this cycle
	void DrawAllInvokers(
		const TArray<TTuple<UFoliageRTInvokerComponent*, int32>>& AssignedInvokers,  float DeltaTime) const;

	// Per-slot draw helpers
	void DrawContinuousForSlot(UTextureRenderTarget2D* RT, int32 SlotIdx,
		const TArray<TTuple<UFoliageRTInvokerComponent*, int32>>& AssignedInvokers) const;

	void DrawImpulseForSlot(UTextureRenderTarget2D* RT, int32 SlotIdx,
		const TArray<TTuple<UFoliageRTInvokerComponent*, int32>>& AssignedInvokers) const;

	// Tiled brush draw — handles cell edge wraparound
	void DrawTiledBrush(UCanvas* Canvas, FVector2D CanvasSize,
						UMaterialInstanceDynamic* MID,
						FVector2D CentreUV, FVector2D UVExtent,
						ESimpleElementBlendMode BlendMode) const;

	// RT clear helpers
	void ProcessPendingImpulseClears() const;
	void ClearContinuousRTs() const;
	void ClearImpulseSlot(int32 SlotIndex) const;
	void ClearContinuousSlot(int32 SlotIndex) const;

	UPROPERTY()
	TObjectPtr<URTPoolManager> PoolManager;



	// New method for progression impulse method
	void DecayImpulseForSlot(UTextureRenderTarget2D* RT, float DeltaTime) const;
	void StampImpulseForSlot(UTextureRenderTarget2D* RT, int32 SlotIdx,
		const TArray<TTuple<UFoliageRTInvokerComponent*, int32>>& AssignedInvokers) const;
};