#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LOSRequirementPoolSubsystem.generated.h"

class UVision_VisualComp;
class UTextureRenderTarget2D;
class UMaterialInstanceDynamic;
class UMaterialInterface;

// ------------------------------------------------------------------ //
//  Pool slot — ObstacleRT + StampMID only
//  VisibilityMesh MIDs are always owned locally by each actor.
// ------------------------------------------------------------------ //

USTRUCT()
struct TOPDOWNVISION_API FLOSStampPoolSlot
{
    GENERATED_BODY()

    UPROPERTY(Transient)
    TObjectPtr<UTextureRenderTarget2D> ObstacleRT = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> StampMID = nullptr;

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

    /** Acquire a slot — hands ObstacleRT and StampMID to the provider. */
    int32 AcquireSlot(UVision_VisualComp* Provider);

    /** Release the slot — strips ObstacleRT and StampMID from the provider. */
    void ReleaseSlot(UVision_VisualComp* Provider);

    void DrainPool();

    int32 GetPoolSize()      const { return Pool.Num(); }
    int32 GetAcquiredCount() const;

private:
    FLOSStampPoolSlot* AllocateSlot();
    void BindSlotToProvider(FLOSStampPoolSlot& Slot, UVision_VisualComp* Provider);
    void UnbindSlotFromProvider(FLOSStampPoolSlot& Slot);

    UTextureRenderTarget2D*   CreateObstacleRT();
    UMaterialInstanceDynamic* CreateStampMID();

private:
    UPROPERTY(Transient)
    TArray<FLOSStampPoolSlot> Pool;
};
