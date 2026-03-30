// Fill out your copyright notice in the Description page of Project Settings.

#include "LineOfSight/Management/Subsystem/LOSRequirementPoolSubsystem.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

#include "TopDownVision/Public/LineOfSight/VisionComps/Vision_VisualComp.h"
#include "LineOfSight/LOSVisual/VisibilityMeshComp.h"
#include "EditorSetting/LOSResourcePoolSettings.h"
#include "TopDownVisionDebug.h"


// ------------------------------------------------------------------ //
//  Lifetime
// ------------------------------------------------------------------ //

void ULOSRequirementPoolSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    const ULOSResourcePoolSettings* Settings = ULOSResourcePoolSettings::Get();
    if (!Settings)
    {
        UE_LOG(LOSVision, Error,
            TEXT("ULOSRequirementPoolSubsystem::Initialize >> LOSResourcePoolSettings not found"));
        return;
    }

    // Pre-warm RT + StampMID pool
    Pool.Reserve(Settings->PreWarmCount);
    for (int32 i = 0; i < Settings->PreWarmCount; ++i)
    {
        if (!AllocateSlot())
        {
            UE_LOG(LOSVision, Warning,
                TEXT("ULOSRequirementPoolSubsystem::Initialize >> RT slot failed at %d — stopping"), i);
            break;
        }
    }

    // Pre-warm visibility MID pools
    PreWarmVisibilityMIDPools();

    UE_LOG(LOSVision, Log,
        TEXT("ULOSRequirementPoolSubsystem::Initialize >> RT slots: %d | MID pools: %d"),
        Pool.Num(), VisibilityMIDPools.Num());
}

void ULOSRequirementPoolSubsystem::Deinitialize()
{
    DrainPool();
    Pool.Empty();
    VisibilityMIDPools.Empty();
    VisibilityPoolIndex.Empty();
    ProviderMIDSlotMap.Empty();
    Super::Deinitialize();
}

// ------------------------------------------------------------------ //
//  Public API
// ------------------------------------------------------------------ //

int32 ULOSRequirementPoolSubsystem::AcquireSlot(UVision_VisualComp* Provider)
{
    if (!Provider)
        return INDEX_NONE;

    // Reclaim stale RT slots
    for (FLOSStampPoolSlot& Slot : Pool)
    {
        if (Slot.IsStale())
        {
            UnbindSlotFromProvider(Slot);
            Slot.bInUse = false;
            Slot.Owner.Reset();
        }
    }

    // Find free RT slot
    FLOSStampPoolSlot* FreeSlot = nullptr;
    int32 FreeIndex = INDEX_NONE;

    for (int32 i = 0; i < Pool.Num(); ++i)
    {
        if (!Pool[i].bInUse && Pool[i].IsValid())
        {
            FreeSlot  = &Pool[i];
            FreeIndex = i;
            break;
        }
    }

    if (!FreeSlot)
    {
        const ULOSResourcePoolSettings* Settings = ULOSResourcePoolSettings::Get();
        if (Settings && Settings->bGrowOnDemand)
        {
            FreeSlot = AllocateSlot();
            if (FreeSlot)
            {
                FreeIndex = Pool.Num() - 1;
                UE_LOG(LOSVision, Log,
                    TEXT("ULOSRequirementPoolSubsystem::AcquireSlot >> RT pool grew to %d"), Pool.Num());
            }
        }

        if (!FreeSlot)
        {
            UE_LOG(LOSVision, Warning,
                TEXT("ULOSRequirementPoolSubsystem::AcquireSlot >> RT pool exhausted — skipping [%s]"),
                Provider->GetOwner() ? *Provider->GetOwner()->GetName() : TEXT("Unknown"));
            return INDEX_NONE;
        }
    }

    FreeSlot->bInUse = true;
    FreeSlot->Owner  = Provider;
    BindSlotToProvider(*FreeSlot, Provider);

    UE_LOG(LOSVision, Verbose,
        TEXT("ULOSRequirementPoolSubsystem::AcquireSlot >> Slot %d acquired for [%s]"),
        FreeIndex,
        Provider->GetOwner() ? *Provider->GetOwner()->GetName() : TEXT("Unknown"));

    return FreeIndex;
}

void ULOSRequirementPoolSubsystem::ReleaseSlot(UVision_VisualComp* Provider)
{
    if (!Provider)
        return;

    for (FLOSStampPoolSlot& Slot : Pool)
    {
        if (Slot.Owner.Get() != Provider)
            continue;

        UnbindSlotFromProvider(Slot);
        Slot.bInUse = false;
        Slot.Owner.Reset();

        UE_LOG(LOSVision, Verbose,
            TEXT("ULOSRequirementPoolSubsystem::ReleaseSlot >> Released from [%s]"),
            Provider->GetOwner() ? *Provider->GetOwner()->GetName() : TEXT("Unknown"));
        return;
    }

    UE_LOG(LOSVision, Warning,
        TEXT("ULOSRequirementPoolSubsystem::ReleaseSlot >> No slot found for [%s]"),
        Provider->GetOwner() ? *Provider->GetOwner()->GetName() : TEXT("Unknown"));
}

void ULOSRequirementPoolSubsystem::DrainPool()
{
    for (FLOSStampPoolSlot& Slot : Pool)
    {
        if (Slot.bInUse)
            UnbindSlotFromProvider(Slot);
        Slot.bInUse = false;
        Slot.Owner.Reset();
    }

    // Reset all MID pool free flags
    for (FLOSVisibilityMIDPool& MIDPool : VisibilityMIDPools)
        for (bool& bFree : MIDPool.bFree)
            bFree = true;

    ProviderMIDSlotMap.Empty();

    UE_LOG(LOSVision, Log,
        TEXT("ULOSRequirementPoolSubsystem::DrainPool >> All slots released"));
}

int32 ULOSRequirementPoolSubsystem::GetAcquiredCount() const
{
    int32 Count = 0;
    for (const FLOSStampPoolSlot& Slot : Pool)
        if (Slot.bInUse) ++Count;
    return Count;
}

// ------------------------------------------------------------------ //
//  Internal — RT + StampMID allocation
// ------------------------------------------------------------------ //

FLOSStampPoolSlot* ULOSRequirementPoolSubsystem::AllocateSlot()
{
    FLOSStampPoolSlot& NewSlot = Pool.AddDefaulted_GetRef();
    NewSlot.ObstacleRT = CreateObstacleRT();
    NewSlot.StampMID   = CreateStampMID();

    if (!NewSlot.IsValid())
    {
        Pool.RemoveAt(Pool.Num() - 1);
        UE_LOG(LOSVision, Error,
            TEXT("ULOSRequirementPoolSubsystem::AllocateSlot >> RT or MID creation failed"));
        return nullptr;
    }

    return &Pool.Last();
}

void ULOSRequirementPoolSubsystem::BindSlotToProvider(FLOSStampPoolSlot& Slot, UVision_VisualComp* Provider)
{
    // Acquire visibility MID set if provider has a key
    if (UVisibilityMeshComp* MeshComp = Provider->GetVisibilityMeshComp())
    {
        const FName MeshKey = MeshComp->GetMeshKey();
        if (MeshKey != NAME_None)
        {
            int32 PoolIdx = INDEX_NONE;
            int32 SetIdx  = INDEX_NONE;
            FLOSVisibilityMIDSet MIDSet = AcquireVisibilityMIDSet(MeshKey, PoolIdx, SetIdx);

            if (!MIDSet.IsEmpty())
            {
                ProviderMIDSlotMap.Add(Provider, MakeTuple(PoolIdx, SetIdx));
                MeshComp->SetMIDsFromPool(MIDSet);
            }
            else
            {
                UE_LOG(LOSVision, Verbose,
                    TEXT("ULOSRequirementPoolSubsystem::BindSlotToProvider >> [%s] MID pool exhausted for key [%s] — original materials kept"),
                    Provider->GetOwner() ? *Provider->GetOwner()->GetName() : TEXT("Unknown"),
                    *MeshKey.ToString());
            }
        }
    }

    Provider->OnPoolSlotAcquired(Slot);
}

void ULOSRequirementPoolSubsystem::UnbindSlotFromProvider(FLOSStampPoolSlot& Slot)
{
    if (UVision_VisualComp* Provider = Slot.Owner.Get())
    {
        // Return MID set to pool
        if (const TTuple<int32, int32>* Entry = ProviderMIDSlotMap.Find(Provider))
        {
            ReleaseVisibilityMIDSet(Entry->Key, Entry->Value);
            ProviderMIDSlotMap.Remove(Provider);

            if (UVisibilityMeshComp* MeshComp = Provider->GetVisibilityMeshComp())
                MeshComp->ClearPoolMIDs();
        }

        Provider->OnPoolSlotReleased();
    }
}

// ------------------------------------------------------------------ //
//  Internal — visibility MID pool
// ------------------------------------------------------------------ //

void ULOSRequirementPoolSubsystem::PreWarmVisibilityMIDPools()
{
    const ULOSResourcePoolSettings* Settings = ULOSResourcePoolSettings::Get();
    if (!Settings) return;

    for (const FLOSVisibilityMeshMaterialSlot& SlotDef : Settings->VisibilityMeshMaterialSlots)
    {
        if (SlotDef.MeshKey == NAME_None || SlotDef.Materials.IsEmpty())
        {
            UE_LOG(LOSVision, Warning,
                TEXT("ULOSRequirementPoolSubsystem::PreWarmVisibilityMIDPools >> Entry with no key or no materials — skipped"));
            continue;
        }

        FLOSVisibilityMIDPool& MIDPool = VisibilityMIDPools.AddDefaulted_GetRef();
        MIDPool.MeshKey = SlotDef.MeshKey;

        const int32 PoolIdx = VisibilityMIDPools.Num() - 1;
        VisibilityPoolIndex.Add(SlotDef.MeshKey, PoolIdx);

        MIDPool.Sets.Reserve(SlotDef.PoolCount);
        MIDPool.bFree.Reserve(SlotDef.PoolCount);

        for (int32 i = 0; i < SlotDef.PoolCount; ++i)
        {
            MIDPool.Sets.Add(CreateMIDSet(SlotDef));
            MIDPool.bFree.Add(true);
        }

        UE_LOG(LOSVision, Log,
            TEXT("ULOSRequirementPoolSubsystem::PreWarmVisibilityMIDPools >> Key [%s] — %d sets pre-warmed"),
            *SlotDef.MeshKey.ToString(), SlotDef.PoolCount);
    }
}

FLOSVisibilityMIDSet ULOSRequirementPoolSubsystem::CreateMIDSet(const FLOSVisibilityMeshMaterialSlot& SlotDef)
{
    FLOSVisibilityMIDSet Set;

    for (const FLOSMIDMaterialEntry& Entry : SlotDef.Materials)
    {
        UMaterialInterface* BaseMat = Entry.Material.LoadSynchronous();
        if (!BaseMat)
        {
            UE_LOG(LOSVision, Warning,
                TEXT("ULOSRequirementPoolSubsystem::CreateMIDSet >> Material at slot %d for key [%s] failed to load"),
                Entry.SlotIndex, *SlotDef.MeshKey.ToString());
            continue;
        }

        Set.SlotIndices.Add(Entry.SlotIndex);
        Set.MIDs.Add(UMaterialInstanceDynamic::Create(BaseMat, this));
    }

    return Set;
}

FLOSVisibilityMIDSet ULOSRequirementPoolSubsystem::AcquireVisibilityMIDSet(
    FName MeshKey, int32& OutPoolIndex, int32& OutSetIndex)
{
    OutPoolIndex = INDEX_NONE;
    OutSetIndex  = INDEX_NONE;

    const int32* PoolIdx = VisibilityPoolIndex.Find(MeshKey);
    if (!PoolIdx)
    {
        UE_LOG(LOSVision, Verbose,
            TEXT("ULOSRequirementPoolSubsystem::AcquireVisibilityMIDSet >> Key [%s] not in settings"),
            *MeshKey.ToString());
        return {};
    }

    FLOSVisibilityMIDPool& MIDPool = VisibilityMIDPools[*PoolIdx];
    const int32 SetIdx = MIDPool.Acquire();

    if (SetIdx == INDEX_NONE)
    {
        UE_LOG(LOSVision, Verbose,
            TEXT("ULOSRequirementPoolSubsystem::AcquireVisibilityMIDSet >> Key [%s] pool exhausted"),
            *MeshKey.ToString());
        return {};
    }

    OutPoolIndex = *PoolIdx;
    OutSetIndex  = SetIdx;

    return MIDPool.Sets[SetIdx];
}

void ULOSRequirementPoolSubsystem::ReleaseVisibilityMIDSet(int32 PoolIndex, int32 SetIndex)
{
    if (VisibilityMIDPools.IsValidIndex(PoolIndex))
        VisibilityMIDPools[PoolIndex].Release(SetIndex);
}

// ------------------------------------------------------------------ //
//  Internal — resource creation
// ------------------------------------------------------------------ //

UTextureRenderTarget2D* ULOSRequirementPoolSubsystem::CreateObstacleRT()
{
    const ULOSResourcePoolSettings* Settings = ULOSResourcePoolSettings::Get();
    if (!Settings) return nullptr;

    UTextureRenderTarget2D* Template = Settings->TemplateObstacleRT.LoadSynchronous();
    if (!Template)
    {
        UE_LOG(LOSVision, Error,
            TEXT("ULOSRequirementPoolSubsystem::CreateObstacleRT >> TemplateObstacleRT not set"));
        return nullptr;
    }

    UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(this);
    RT->RenderTargetFormat = Template->RenderTargetFormat;
    RT->ClearColor         = Template->ClearColor;
    RT->bAutoGenerateMips  = Template->bAutoGenerateMips;
    RT->InitAutoFormat(Template->SizeX, Template->SizeY);
    RT->UpdateResource();

    return RT;
}

UMaterialInstanceDynamic* ULOSRequirementPoolSubsystem::CreateStampMID()
{
    const ULOSResourcePoolSettings* Settings = ULOSResourcePoolSettings::Get();
    if (!Settings) return nullptr;

    UMaterialInterface* BaseMat = Settings->LOSStampMaterial.LoadSynchronous();
    if (!BaseMat)
    {
        UE_LOG(LOSVision, Error,
            TEXT("ULOSRequirementPoolSubsystem::CreateStampMID >> LOSStampMaterial not set"));
        return nullptr;
    }

    return UMaterialInstanceDynamic::Create(BaseMat, this);
}
