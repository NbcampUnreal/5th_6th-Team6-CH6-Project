// Fill out your copyright notice in the Description page of Project Settings.

#include "LineOfSight/Management/Subsystem/LOSRequirementPoolSubsystem.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

#include "TopDownVision/Public/LineOfSight/VisionComps/Vision_VisualComp.h"
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

    Pool.Reserve(Settings->PreWarmCount);

    for (int32 i = 0; i < Settings->PreWarmCount; ++i)
    {
        FLOSStampPoolSlot* Slot = AllocateSlot();
        if (!Slot)
        {
            UE_LOG(LOSVision, Warning,
                TEXT("ULOSRequirementPoolSubsystem::Initialize >> Failed at slot %d — stopping pre-warm"), i);
            break;
        }
    }

    UE_LOG(LOSVision, Log,
        TEXT("ULOSRequirementPoolSubsystem::Initialize >> Pre-warmed %d / %d slots"),
        Pool.Num(), Settings->PreWarmCount);
}

void ULOSRequirementPoolSubsystem::Deinitialize()
{
    DrainPool();
    Pool.Empty();
    Super::Deinitialize();
}

// ------------------------------------------------------------------ //
//  Public API
// ------------------------------------------------------------------ //

int32 ULOSRequirementPoolSubsystem::AcquireSlot(UVision_VisualComp* Provider)
{
    if (!Provider)
        return INDEX_NONE;

    // Reclaim stale slots
    for (FLOSStampPoolSlot& Slot : Pool)
    {
        if (Slot.IsStale())
        {
            UE_LOG(LOSVision, Verbose,
                TEXT("ULOSRequirementPoolSubsystem::AcquireSlot >> Reclaiming stale slot"));
            UnbindSlotFromProvider(Slot);
            Slot.bInUse = false;
            Slot.Owner.Reset();
        }
    }

    // Find a free slot
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

    // Grow if needed
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
                    TEXT("ULOSRequirementPoolSubsystem::AcquireSlot >> Pool grew to %d slots"), Pool.Num());
            }
        }

        if (!FreeSlot)
        {
            UE_LOG(LOSVision, Warning,
                TEXT("ULOSRequirementPoolSubsystem::AcquireSlot >> Pool exhausted — skipping [%s]"),
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
//  Internal — allocation
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
            TEXT("ULOSRequirementPoolSubsystem::AllocateSlot >> RT or MID creation failed — slot discarded"));
        return nullptr;
    }

    return &Pool.Last();
}

// ------------------------------------------------------------------ //
//  Internal — bind / unbind
//  Subsystem only hands RT and StampMID to the provider.
//  VisibilityMesh is always locally owned — never touched here.
// ------------------------------------------------------------------ //

void ULOSRequirementPoolSubsystem::BindSlotToProvider(FLOSStampPoolSlot& Slot, UVision_VisualComp* Provider)
{
    Provider->OnPoolSlotAcquired(Slot);
}

void ULOSRequirementPoolSubsystem::UnbindSlotFromProvider(FLOSStampPoolSlot& Slot)
{
    if (UVision_VisualComp* Provider = Slot.Owner.Get())
        Provider->OnPoolSlotReleased();
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
