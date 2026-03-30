#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/RTPoolTypes.h"
#include "RTPoolManager.generated.h"

class UTextureRenderTarget2D;
class UFoliageRTInvokerComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCellAssigned,
    FIntPoint, CellIndex, int32, PoolSlotIndex);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCellReclaimed,
    FIntPoint, CellIndex, int32, PoolSlotIndex);

UCLASS(BlueprintType)
class MATERIALDRIVENINTERACTION_API URTPoolManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

    void Tick(float DeltaTime);

    void SetPriorityCenter(FVector WorldLocation);
    FVector GetPriorityCenter() const { return PriorityCenter; }

    void SetDrawRadius(float Radius) { DrawRadius = Radius; }
    float GetDrawRadius() const { return DrawRadius; }

    void RegisterInvoker(UFoliageRTInvokerComponent* Invoker);
    void UnregisterInvoker(UFoliageRTInvokerComponent* Invoker);

    TArray<TTuple<UFoliageRTInvokerComponent*, int32>> EvaluateAndAssignSlots();
    void ReclaimExpiredSlots();
    void ClearSlotFlag(int32 SlotIndex);

    UFUNCTION(BlueprintCallable, Category = "Foliage RT|Query")
    bool GetRTForWorldPosition(FVector WorldLocation,
                               UTextureRenderTarget2D*& OutInteractionRT,
                               FVector2D& OutUV) const;

    UFUNCTION(BlueprintCallable, Category = "Foliage RT|Query")
    bool GetSlotData(int32 SlotIndex,
                     UTextureRenderTarget2D*& OutInteractionRT,
                     FVector2D& OutCellOriginWS) const;

    UFUNCTION(BlueprintPure, Category = "Foliage RT|Utility")
    FIntPoint WorldToCell(FVector WorldLocation) const;

    UFUNCTION(BlueprintPure, Category = "Foliage RT|Utility")
    FVector2D CellToWorldOrigin(FIntPoint CellIndex) const;

    UFUNCTION(BlueprintPure, Category = "Foliage RT|Debug")
    int32 GetActiveSlotCount() const;

    UFUNCTION(BlueprintPure, Category = "Foliage RT|Debug")
    const TArray<FRTPoolEntry>& GetPool() const { return Pool; }

    UFUNCTION(BlueprintPure, Category = "Foliage RT|Debug")
    FString GetPoolDebugString() const;

    bool IsInvokerActive(UFoliageRTInvokerComponent* Invoker) const;

    UPROPERTY(BlueprintAssignable, Category = "Foliage RT|Events")
    FOnCellAssigned OnCellAssigned;

    UPROPERTY(BlueprintAssignable, Category = "Foliage RT|Events")
    FOnCellReclaimed OnCellReclaimed;

    UPROPERTY(BlueprintReadOnly, Category = "Foliage RT|Config")
    int32 PoolSize = 9;

    UPROPERTY(BlueprintReadOnly, Category = "Foliage RT|Config")
    float CellSize = 2000.f;

    UPROPERTY(BlueprintReadOnly, Category = "Foliage RT|Config")
    int32 RTResolution = 512;

    UPROPERTY(BlueprintReadOnly, Category = "Foliage RT|Config")
    float DecayDuration = 10.f;

private:

    UTextureRenderTarget2D* LoadInteractionRT(int32 SlotIndex) const;

    int32 AssignFreeSlot();
    int32 EvictBestSlot();

    UPROPERTY(Transient)
    TArray<FRTPoolEntry> Pool;

    UPROPERTY(Transient)
    TArray<TWeakObjectPtr<UFoliageRTInvokerComponent>> RegisteredInvokers;



    FVector PriorityCenter = FVector::ZeroVector;
    uint64  LastTickFrame  = 0;
    float   DrawRadius     = 5000.f;
    float CachedDecayRate = 0.95f;
};