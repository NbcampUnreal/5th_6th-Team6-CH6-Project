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

    UFUNCTION(BlueprintCallable, Category = "RTDrawManager")
    void Update(float DeltaTime);
    


private:

    void DrawAllInvokers(
        const TArray<TTuple<UFoliageRTInvokerComponent*, int32>>& AssignedInvokers) const;

    void DrawSlot(UTextureRenderTarget2D* RT, int32 SlotIdx,
        const TArray<TTuple<UFoliageRTInvokerComponent*, int32>>& AssignedInvokers) const;
    

    void DrawBrush(UCanvas* Canvas, FVector2D CanvasSize,
        UMaterialInstanceDynamic* MID,
        FVector2D CentreUV, FVector2D UVExtent) const;

    void ProcessPendingClears() const;
    void ClearSlot(int32 SlotIndex) const;

    UPROPERTY(Transient)
    TObjectPtr<URTPoolManager> PoolManager;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> DecayMID;
};