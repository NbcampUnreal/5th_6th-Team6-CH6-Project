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

    // Full draw cycle:
    //   1. Pool evaluates invokers and assigns slots by priority
    //   2. Clear pending ImpulseRTs (on slot reassignment)
    //   3. Clear ContinuousRTs for occupied slots
    //   4. Draw all invokers — Continuous and Impulse every tick via SE_BLEND_MAX
    //   5. Reclaim expired slots
    UFUNCTION(BlueprintCallable, Category = "RTDrawManager")
    void Update(float DeltaTime);

private:

    void DrawAllInvokers(
        const TArray<TTuple<UFoliageRTInvokerComponent*, int32>>& AssignedInvokers,
        float DeltaTime) const;

    void DrawContinuousForSlot(UTextureRenderTarget2D* RT, int32 SlotIdx,
        const TArray<TTuple<UFoliageRTInvokerComponent*, int32>>& AssignedInvokers) const;

    // Impulse drawn every tick — SE_BLEND_MAX preserves highest (latest)
    // timestamp per texel. Freezes naturally when invoker leaves.
    // No decay pass, no stamp gating.
    void DrawImpulseForSlot(UTextureRenderTarget2D* RT, int32 SlotIdx,
        const TArray<TTuple<UFoliageRTInvokerComponent*, int32>>& AssignedInvokers) const;

    void DrawTiledBrush(UCanvas* Canvas, FVector2D CanvasSize,
        UMaterialInstanceDynamic* MID,
        FVector2D CentreUV, FVector2D UVExtent,
        ESimpleElementBlendMode BlendMode) const;

    void ProcessPendingImpulseClears() const;
    void ClearContinuousRTs() const;
    void ClearImpulseSlot(int32 SlotIndex) const;
    void ClearContinuousSlot(int32 SlotIndex) const;

    // DecayImpulseForSlot removed — no feedback loop needed
    // StampImpulseForSlot removed — MAX blend handles preservation

    UPROPERTY()
    TObjectPtr<URTPoolManager> PoolManager;
};