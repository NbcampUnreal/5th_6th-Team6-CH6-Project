// Fill out your copyright notice in the Description page of Project Settings.

#include "LineOfSight/LOSVisual/VisibilityMeshComp.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TopDownVisionDebug.h"

DEFINE_LOG_CATEGORY(VisibilityMeshComp);


UVisibilityMeshComp::UVisibilityMeshComp()
{
    PrimaryComponentTick.bCanEverTick = false;
}

// -------------------------------------------------------------------------- //
//  Editor utility
// -------------------------------------------------------------------------- //

void UVisibilityMeshComp::FindMeshesByTag()
{
    AActor* Actor = GetOwner();
    if (!Actor)
    {
        UE_LOG(VisibilityMeshComp, Warning,
            TEXT("UVisibilityMeshComp::FindMeshesByTag >> No owning actor"));
        return;
    }

    SkeletalMeshTargets.Empty();
    StaticMeshTargets.Empty();

    TArray<USkeletalMeshComponent*> SkelMeshes;
    Actor->GetComponents<USkeletalMeshComponent>(SkelMeshes);
    for (USkeletalMeshComponent* Mesh : SkelMeshes)
    {
        if (!Mesh || !Mesh->ComponentHasTag(VisibilityMeshTag))
            continue;

        SkeletalMeshTargets.Add(TSoftObjectPtr<USkeletalMeshComponent>(Mesh));

        UE_LOG(VisibilityMeshComp, Verbose,
            TEXT("UVisibilityMeshComp::FindMeshesByTag >> Skeletal: %s (%d slots)"),
            *Mesh->GetFName().ToString(), Mesh->GetNumMaterials());
    }

    TArray<UStaticMeshComponent*> StaticMeshes;
    Actor->GetComponents<UStaticMeshComponent>(StaticMeshes);
    for (UStaticMeshComponent* Mesh : StaticMeshes)
    {
        if (!Mesh || !Mesh->ComponentHasTag(VisibilityMeshTag))
            continue;

        StaticMeshTargets.Add(TSoftObjectPtr<UStaticMeshComponent>(Mesh));

        UE_LOG(VisibilityMeshComp, Verbose,
            TEXT("UVisibilityMeshComp::FindMeshesByTag >> Static: %s (%d slots)"),
            *Mesh->GetFName().ToString(), Mesh->GetNumMaterials());
    }

    UE_LOG(VisibilityMeshComp, Verbose,
        TEXT("UVisibilityMeshComp::FindMeshesByTag >> Resolved %d skeletal, %d static"),
        SkeletalMeshTargets.Num(), StaticMeshTargets.Num());

    // Cache originals immediately — before any SetMaterial call
    CacheOriginalMaterials();
}

// -------------------------------------------------------------------------- //
//  Manual registration
// -------------------------------------------------------------------------- //

void UVisibilityMeshComp::AddMesh(TSoftObjectPtr<USkeletalMeshComponent> Mesh)
{
    if (Mesh.IsNull())
    {
        UE_LOG(VisibilityMeshComp, Warning,
            TEXT("[%s] UVisibilityMeshComp::AddMesh >> Null skeletal soft ptr passed"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        return;
    }
    SkeletalMeshTargets.AddUnique(Mesh);
    CacheOriginalMaterials();
}

void UVisibilityMeshComp::AddMesh(TSoftObjectPtr<UStaticMeshComponent> Mesh)
{
    if (Mesh.IsNull())
    {
        UE_LOG(VisibilityMeshComp, Warning,
            TEXT("[%s] UVisibilityMeshComp::AddMesh >> Null static soft ptr passed"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        return;
    }
    StaticMeshTargets.AddUnique(Mesh);
    CacheOriginalMaterials();
}

// -------------------------------------------------------------------------- //
//  CacheOriginalMaterials
// -------------------------------------------------------------------------- //

void UVisibilityMeshComp::CacheOriginalMaterials()
{
    OriginalMaterials.Empty();

    auto Cache = [&](UMeshComponent* Mesh)
    {
        if (!Mesh) return;
        for (int32 i = 0; i < Mesh->GetNumMaterials(); ++i)
            OriginalMaterials.Add(Mesh->GetMaterial(i));
    };

    for (TSoftObjectPtr<USkeletalMeshComponent>& SoftMesh : SkeletalMeshTargets)
        if (SoftMesh.Get()) Cache(SoftMesh.Get());

    for (TSoftObjectPtr<UStaticMeshComponent>& SoftMesh : StaticMeshTargets)
        if (SoftMesh.Get()) Cache(SoftMesh.Get());

    UE_LOG(VisibilityMeshComp, Verbose,
        TEXT("[%s] UVisibilityMeshComp::CacheOriginalMaterials >> Cached %d materials"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()), OriginalMaterials.Num());
}

// -------------------------------------------------------------------------- //
//  Initialize — owned mode
// -------------------------------------------------------------------------- //

void UVisibilityMeshComp::Initialize()
{
    if (SkeletalMeshTargets.IsEmpty() && StaticMeshTargets.IsEmpty())
    {
        UE_LOG(VisibilityMeshComp, Warning,
            TEXT("[%s] UVisibilityMeshComp::Initialize >> No mesh targets. Run FindMeshesByTag or call AddMesh first."),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        return;
    }

    // Safety — in case FindMeshesByTag wasn't called in editor
    if (OriginalMaterials.IsEmpty())
        CacheOriginalMaterials();

    MIDs.Empty();

    auto ProcessMesh = [&](UMeshComponent* Mesh)
    {
        if (!Mesh) return;
        for (int32 i = 0; i < Mesh->GetNumMaterials(); ++i)
        {
            if (UMaterialInstanceDynamic* MID = Mesh->CreateDynamicMaterialInstance(i))
            {
                MIDs.Add(MID);

                UE_LOG(VisibilityMeshComp, Verbose,
                    TEXT("[%s] UVisibilityMeshComp::Initialize >> %s slot[%d] MID created"),
                    *TopDownVisionDebug::GetClientDebugName(GetOwner()),
                    *Mesh->GetFName().ToString(), i);
            }
        }
    };

    for (TSoftObjectPtr<USkeletalMeshComponent>& SoftMesh : SkeletalMeshTargets)
    {
        if (!SoftMesh.Get())
        {
            UE_LOG(VisibilityMeshComp, Warning,
                TEXT("[%s] UVisibilityMeshComp::Initialize >> Skeletal soft ptr failed to resolve"),
                *TopDownVisionDebug::GetClientDebugName(GetOwner()));
            continue;
        }
        ProcessMesh(SoftMesh.Get());
    }

    for (TSoftObjectPtr<UStaticMeshComponent>& SoftMesh : StaticMeshTargets)
    {
        if (!SoftMesh.Get())
        {
            UE_LOG(VisibilityMeshComp, Warning,
                TEXT("[%s] UVisibilityMeshComp::Initialize >> Static soft ptr failed to resolve"),
                *TopDownVisionDebug::GetClientDebugName(GetOwner()));
            continue;
        }
        ProcessMesh(SoftMesh.Get());
    }

    UpdateVisibility(0.f);

    UE_LOG(VisibilityMeshComp, Verbose,
        TEXT("[%s] UVisibilityMeshComp::Initialize >> %d MIDs created (owned mode)"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()), MIDs.Num());
}

// -------------------------------------------------------------------------- //
//  SetMIDs — pool mode
// -------------------------------------------------------------------------- //

void UVisibilityMeshComp::SetMIDs(const TArray<TObjectPtr<UMaterialInstanceDynamic>>& InMIDs)
{
    // Safety — build cache before first SetMaterial if FindMeshesByTag wasn't called
    if (OriginalMaterials.IsEmpty() && !InMIDs.IsEmpty())
    {
        UE_LOG(VisibilityMeshComp, Warning,
            TEXT("[%s] UVisibilityMeshComp::SetMIDs >> OriginalMaterials cache empty — building now. Run FindMeshesByTag in editor to avoid this."),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        CacheOriginalMaterials();
    }

    MIDs = InMIDs;
    ApplyMIDsToMeshes(MIDs);

    if (MIDs.IsEmpty())
    {
        UE_LOG(VisibilityMeshComp, Verbose,
            TEXT("[%s] UVisibilityMeshComp::SetMIDs >> Originals restored (slot released)"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
    }
    else
    {
        UpdateVisibility(0.f);

        UE_LOG(VisibilityMeshComp, Verbose,
            TEXT("[%s] UVisibilityMeshComp::SetMIDs >> %d MIDs bound (pool mode)"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()), MIDs.Num());
    }
}

// -------------------------------------------------------------------------- //
//  ApplyMIDsToMeshes
// -------------------------------------------------------------------------- //

void UVisibilityMeshComp::ApplyMIDsToMeshes(const TArray<TObjectPtr<UMaterialInstanceDynamic>>& InMIDs)
{
    if (InMIDs.IsEmpty())
    {
        RestoreOriginalMaterials();
        return;
    }

    int32 MIDIndex = 0;

    auto Apply = [&](UMeshComponent* Mesh)
    {
        if (!Mesh) return;
        for (int32 SlotIndex = 0; SlotIndex < Mesh->GetNumMaterials(); ++SlotIndex)
        {
            if (InMIDs.IsValidIndex(MIDIndex))
            {
                Mesh->SetMaterial(SlotIndex, InMIDs[MIDIndex]);
            }
            else
            {
                UE_LOG(VisibilityMeshComp, Warning,
                    TEXT("[%s] UVisibilityMeshComp::ApplyMIDsToMeshes >> MID index %d out of range for %s slot %d — check settings material count"),
                    *TopDownVisionDebug::GetClientDebugName(GetOwner()),
                    MIDIndex, *Mesh->GetFName().ToString(), SlotIndex);
            }
            ++MIDIndex;
        }
    };

    for (TSoftObjectPtr<USkeletalMeshComponent>& SoftMesh : SkeletalMeshTargets)
        if (SoftMesh.Get()) Apply(SoftMesh.Get());

    for (TSoftObjectPtr<UStaticMeshComponent>& SoftMesh : StaticMeshTargets)
        if (SoftMesh.Get()) Apply(SoftMesh.Get());
}

// -------------------------------------------------------------------------- //
//  RestoreOriginalMaterials
// -------------------------------------------------------------------------- //

void UVisibilityMeshComp::RestoreOriginalMaterials()
{
    if (OriginalMaterials.IsEmpty())
    {
        UE_LOG(VisibilityMeshComp, Warning,
            TEXT("[%s] UVisibilityMeshComp::RestoreOriginalMaterials >> Cache empty — was CacheOriginalMaterials called before SetMaterial?"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        return;
    }

    int32 OriginalIndex = 0;

    auto Restore = [&](UMeshComponent* Mesh)
    {
        if (!Mesh) return;
        for (int32 SlotIndex = 0; SlotIndex < Mesh->GetNumMaterials(); ++SlotIndex)
        {
            if (OriginalMaterials.IsValidIndex(OriginalIndex))
                Mesh->SetMaterial(SlotIndex, OriginalMaterials[OriginalIndex]);
            ++OriginalIndex;
        }
    };

    for (TSoftObjectPtr<USkeletalMeshComponent>& SoftMesh : SkeletalMeshTargets)
        if (SoftMesh.Get()) Restore(SoftMesh.Get());

    for (TSoftObjectPtr<UStaticMeshComponent>& SoftMesh : StaticMeshTargets)
        if (SoftMesh.Get()) Restore(SoftMesh.Get());

    UE_LOG(VisibilityMeshComp, Verbose,
        TEXT("[%s] UVisibilityMeshComp::RestoreOriginalMaterials >> Restored %d slots"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()), OriginalIndex);
}

// -------------------------------------------------------------------------- //
//  UpdateVisibility
// -------------------------------------------------------------------------- //

void UVisibilityMeshComp::UpdateVisibility(float Alpha)
{
    if (VisibilityParam == NAME_None)
    {
        UE_LOG(VisibilityMeshComp, Warning,
            TEXT("[%s] UVisibilityMeshComp::UpdateVisibility >> VisibilityParam not set"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        return;
    }

    const float Clamped = FMath::Clamp(Alpha, 0.f, 1.f);

    for (TObjectPtr<UMaterialInstanceDynamic>& MID : MIDs)
    {
        if (MID)
            MID->SetScalarParameterValue(VisibilityParam, Clamped);
    }
}
