#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LOSRequirementPoolSubsystem.generated.h"

class UVision_VisualComp;
class UVisibilityMeshComp;
class UTextureRenderTarget2D;
class UMaterialInstanceDynamic;
class UMaterialInterface;

// ── RT + StampMID slot ───────────────────────────────────────────────────────

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

// ── Visibility MID set ───────────────────────────────────────────────────────

/**
 * One pre-created set of MIDs for a single actor instance.
 * SlotIndices and MIDs are parallel arrays — SlotIndices[i] is the mesh slot
 * that MIDs[i] applies to.
 */
USTRUCT()
struct TOPDOWNVISION_API FLOSVisibilityMIDSet
{
    GENERATED_BODY()

    UPROPERTY(Transient)
    TArray<int32> SlotIndices;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInstanceDynamic>> MIDs;

    bool IsEmpty() const { return MIDs.IsEmpty(); }
};

// ── Per-key fixed-size MID pool ───────────────────────────────────────────────

struct FLOSVisibilityMIDPool
{
    FName MeshKey;

    /** Pre-created sets — fixed size, never grows. */
    TArray<FLOSVisibilityMIDSet> Sets;

    /** Which sets are free. Parallel to Sets. */
    TArray<bool> bFree;

    /** Returns index of a free set, or INDEX_NONE if pool exhausted. */
    int32 Acquire()
    {
        for (int32 i = 0; i < bFree.Num(); ++i)
            if (bFree[i]) { bFree[i] = false; return i; }
        return INDEX_NONE;
    }

    void Release(int32 Index)
    {
        if (bFree.IsValidIndex(Index))
            bFree[Index] = true;
    }
};

// ── Subsystem ────────────────────────────────────────────────────────────────

UCLASS()
class TOPDOWNVISION_API ULOSRequirementPoolSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    int32 AcquireSlot(UVision_VisualComp* Provider);
    void  ReleaseSlot(UVision_VisualComp* Provider);
    void  DrainPool();

    int32 GetPoolSize()      const { return Pool.Num(); }
    int32 GetAcquiredCount() const;

private:
    // RT + StampMID
    FLOSStampPoolSlot* AllocateSlot();
    void BindSlotToProvider(FLOSStampPoolSlot& Slot, UVision_VisualComp* Provider);
    void UnbindSlotFromProvider(FLOSStampPoolSlot& Slot);

    UTextureRenderTarget2D*   CreateObstacleRT();
    UMaterialInstanceDynamic* CreateStampMID();

    // Visibility MID pool
    void PreWarmVisibilityMIDPools();
    FLOSVisibilityMIDSet CreateMIDSet(const struct FLOSVisibilityMeshMaterialSlot& SlotDef);

    /** Acquire a MID set for the given key. Returns empty set if pool exhausted. */
    FLOSVisibilityMIDSet AcquireVisibilityMIDSet(FName MeshKey, int32& OutPoolIndex, int32& OutSetIndex);

    /** Return a MID set to its pool. */
    void ReleaseVisibilityMIDSet(int32 PoolIndex, int32 SetIndex);

private:
    UPROPERTY(Transient)
    TArray<FLOSStampPoolSlot> Pool;

    /** One pool per MeshKey — fixed size, pre-warmed at Initialize. */
    TArray<FLOSVisibilityMIDPool> VisibilityMIDPools;

    /** MeshKey → index into VisibilityMIDPools. */
    TMap<FName, int32> VisibilityPoolIndex;

    /** Tracks which pool+set each provider holds for clean release. */
    TMap<UVision_VisualComp*, TTuple<int32, int32>> ProviderMIDSlotMap;
};
