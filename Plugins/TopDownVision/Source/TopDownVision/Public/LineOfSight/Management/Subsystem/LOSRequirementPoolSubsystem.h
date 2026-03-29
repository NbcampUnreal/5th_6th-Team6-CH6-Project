#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LOSRequirementPoolSubsystem.generated.h"

class UVision_VisualComp;
class UTextureRenderTarget2D;
class UMaterialInstanceDynamic;
class UMaterialInterface;

// ------------------------------------------------------------------ //
//  Pool slot
// ------------------------------------------------------------------ //

USTRUCT()
struct TOPDOWNVISION_API FLOSStampPoolSlot
{
    GENERATED_BODY()

    /** Obstacle mask RT — handed to ObstacleDrawer's sampler on acquire. */
    UPROPERTY(Transient)
    TObjectPtr<UTextureRenderTarget2D> ObstacleRT = nullptr;

    /** LOS raymarching stamp MID — handed to StampDrawer on acquire. */
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> StampMID = nullptr;

    /** Visibility mesh MIDs — built at acquire time from the provider's MeshKey.
     *  Empty if MeshKey is NAME_None or unregistered in settings. */
    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> VisibilityMeshMIDs;

    /** Which provider currently holds this slot. Weak — dead providers auto-invalidate. */
    UPROPERTY(Transient)
    TWeakObjectPtr<UVision_VisualComp> Owner;

    bool bInUse = false;

    bool IsValid() const { return ObstacleRT != nullptr && StampMID != nullptr; }
    bool IsStale() const { return bInUse && !Owner.IsValid(); }
};

// ------------------------------------------------------------------ //
//  Subsystem
// ------------------------------------------------------------------ //

UCLASS()
class TOPDOWNVISION_API ULOSRequirementPoolSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Find a free slot and call Provider->OnPoolSlotAcquired.
     *  Returns slot index, or INDEX_NONE if pool is exhausted and bGrowOnDemand=false. */
    int32 AcquireSlot(UVision_VisualComp* Provider);

    /** Call Provider->OnPoolSlotReleased and return the slot to the free list. */
    void ReleaseSlot(UVision_VisualComp* Provider);

    /** Release all acquired slots — call on level transition or manager shutdown. */
    void DrainPool();

    int32 GetPoolSize()      const { return Pool.Num(); }
    int32 GetAcquiredCount() const;

private:
    FLOSStampPoolSlot* AllocateSlot();
    void BindSlotToProvider(FLOSStampPoolSlot& Slot, UVision_VisualComp* Provider);
    void UnbindSlotFromProvider(FLOSStampPoolSlot& Slot);

    UTextureRenderTarget2D*                      CreateObstacleRT();
    UMaterialInstanceDynamic*                    CreateStampMID();
    TArray<TObjectPtr<UMaterialInstanceDynamic>> CreateVisibilityMeshMIDs(FName MeshKey);

private:
    UPROPERTY(Transient)
    TArray<FLOSStampPoolSlot> Pool;
};
