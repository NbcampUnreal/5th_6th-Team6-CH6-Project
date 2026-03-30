// Fill out your copyright notice in the Description page of Project Settings.

#include "LineOfSight/LOSVisual/VisibilityMeshComp.h"
#include "LineOfSight/Management/Subsystem/LOSRequirementPoolSubsystem.h"

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
//  SetMeshKey — triggers FindMeshesByTag at runtime after data loads
// -------------------------------------------------------------------------- //

void UVisibilityMeshComp::SetMeshKey(FName InMeshKey)
{
    MeshKey = InMeshKey;
    FindMeshesByTag();

    UE_LOG(VisibilityMeshComp, Verbose,
        TEXT("[%s] UVisibilityMeshComp::SetMeshKey >> [%s] — FindMeshesByTag triggered"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()),
        *InMeshKey.ToString());
}

// -------------------------------------------------------------------------- //
//  FindMeshesByTag
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
    if (Mesh.IsNull()) return;
    SkeletalMeshTargets.AddUnique(Mesh);
    CacheOriginalMaterials();
}

void UVisibilityMeshComp::AddMesh(TSoftObjectPtr<UStaticMeshComponent> Mesh)
{
    if (Mesh.IsNull()) return;
    StaticMeshTargets.AddUnique(Mesh);
    CacheOriginalMaterials();
}

// -------------------------------------------------------------------------- //
//  CacheOriginalMaterials
//  Stores (mesh component, slot index, original material) before any SetMaterial.
//  Since pool MIDs are applied by explicit slot index, we cache per slot index
//  across all tagged meshes so ClearPoolMIDs can restore each one exactly.
// -------------------------------------------------------------------------- //

void UVisibilityMeshComp::CacheOriginalMaterials()
{
    OriginalMaterials.Empty();
    OriginalSlotIndices.Empty();

    auto Cache = [&](UMeshComponent* Mesh)
    {
        if (!IsValid(Mesh)) return;
        for (int32 i = 0; i < Mesh->GetNumMaterials(); ++i)
        {
            OriginalSlotIndices.Add(i);
            OriginalMaterials.Add(Mesh->GetMaterial(i));
        }
    };

    for (TSoftObjectPtr<USkeletalMeshComponent>& SoftMesh : SkeletalMeshTargets)
        if (SoftMesh.IsValid()) Cache(SoftMesh.Get());

    for (TSoftObjectPtr<UStaticMeshComponent>& SoftMesh : StaticMeshTargets)
        if (SoftMesh.IsValid()) Cache(SoftMesh.Get());

    UE_LOG(VisibilityMeshComp, Verbose,
        TEXT("[%s] UVisibilityMeshComp::CacheOriginalMaterials >> Cached %d slots"),
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
            TEXT("[%s] UVisibilityMeshComp::Initialize >> No mesh targets."),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        return;
    }

    if (OriginalMaterials.IsEmpty())
        CacheOriginalMaterials();

    MIDs.Empty();
    bMIDsArePooled = false;

    auto ProcessMesh = [&](UMeshComponent* Mesh)
    {
        if (!IsValid(Mesh)) return;
        for (int32 i = 0; i < Mesh->GetNumMaterials(); ++i)
        {
            if (UMaterialInstanceDynamic* MID = Mesh->CreateDynamicMaterialInstance(i))
                MIDs.Add(MID);
        }
    };

    for (TSoftObjectPtr<USkeletalMeshComponent>& SoftMesh : SkeletalMeshTargets)
        if (SoftMesh.IsValid()) ProcessMesh(SoftMesh.Get());

    for (TSoftObjectPtr<UStaticMeshComponent>& SoftMesh : StaticMeshTargets)
        if (SoftMesh.IsValid()) ProcessMesh(SoftMesh.Get());

    UpdateVisibility(0.f);

    UE_LOG(VisibilityMeshComp, Verbose,
        TEXT("[%s] UVisibilityMeshComp::Initialize >> %d MIDs created (owned)"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()), MIDs.Num());
}

// -------------------------------------------------------------------------- //
//  Pool mode — SetMIDsFromPool
//  Applies each MID to its specified slot index on the first tagged mesh.
//  Since there is one skeletal mesh per monster, we target the first one.
// -------------------------------------------------------------------------- //

void UVisibilityMeshComp::SetMIDsFromPool(const FLOSVisibilityMIDSet& InSet)
{
    if (InSet.IsEmpty())
    {
        UE_LOG(VisibilityMeshComp, Warning,
            TEXT("[%s] UVisibilityMeshComp::SetMIDsFromPool >> Empty MID set"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        return;
    }

    if (OriginalMaterials.IsEmpty())
    {
        UE_LOG(VisibilityMeshComp, Warning,
            TEXT("[%s] UVisibilityMeshComp::SetMIDsFromPool >> OriginalMaterials cache empty — call SetMeshKey() after data loads"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        CacheOriginalMaterials();
    }

    // Find the first valid tagged skeletal mesh — one per monster
    USkeletalMeshComponent* TargetMesh = nullptr;
    for (TSoftObjectPtr<USkeletalMeshComponent>& SoftMesh : SkeletalMeshTargets)
    {
        if (SoftMesh.IsValid())
        {
            TargetMesh = SoftMesh.Get();
            break;
        }
    }

    if (!IsValid(TargetMesh))
    {
        UE_LOG(VisibilityMeshComp, Warning,
            TEXT("[%s] UVisibilityMeshComp::SetMIDsFromPool >> No valid skeletal mesh found"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        return;
    }

    MIDs.Empty();
    bMIDsArePooled = true;

    for (int32 i = 0; i < InSet.SlotIndices.Num(); ++i)
    {
        const int32 SlotIndex = InSet.SlotIndices[i];
        UMaterialInstanceDynamic* MID = IsValid(InSet.MIDs[i]) ? InSet.MIDs[i].Get() : nullptr;

        if (!MID)
        {
            UE_LOG(VisibilityMeshComp, Warning,
                TEXT("[%s] UVisibilityMeshComp::SetMIDsFromPool >> MID[%d] invalid — skipping slot %d"),
                *TopDownVisionDebug::GetClientDebugName(GetOwner()), i, SlotIndex);
            continue;
        }

        if (SlotIndex < TargetMesh->GetNumMaterials())
        {
            TargetMesh->SetMaterial(SlotIndex, MID);
            MIDs.Add(MID);
        }
        else
        {
            UE_LOG(VisibilityMeshComp, Warning,
                TEXT("[%s] UVisibilityMeshComp::SetMIDsFromPool >> SlotIndex %d out of range (%d slots)"),
                *TopDownVisionDebug::GetClientDebugName(GetOwner()),
                SlotIndex, TargetMesh->GetNumMaterials());
        }
    }

    UpdateVisibility(0.f);

    UE_LOG(VisibilityMeshComp, Verbose,
        TEXT("[%s] UVisibilityMeshComp::SetMIDsFromPool >> %d MIDs applied"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()), MIDs.Num());
}

// -------------------------------------------------------------------------- //
//  Pool mode — ClearPoolMIDs
//  Restores original materials on each slot that was overwritten.
// -------------------------------------------------------------------------- //

void UVisibilityMeshComp::ClearPoolMIDs()
{
    if (!bMIDsArePooled)
        return;

    USkeletalMeshComponent* TargetMesh = nullptr;
    for (TSoftObjectPtr<USkeletalMeshComponent>& SoftMesh : SkeletalMeshTargets)
    {
        if (SoftMesh.IsValid())
        {
            TargetMesh = SoftMesh.Get();
            break;
        }
    }

    if (IsValid(TargetMesh))
    {
        for (int32 i = 0; i < OriginalSlotIndices.Num(); ++i)
        {
            const int32 SlotIndex = OriginalSlotIndices[i];
            if (OriginalMaterials.IsValidIndex(i) && SlotIndex < TargetMesh->GetNumMaterials())
                TargetMesh->SetMaterial(SlotIndex, OriginalMaterials[i]);
        }
    }

    MIDs.Empty();
    bMIDsArePooled = false;

    UE_LOG(VisibilityMeshComp, Verbose,
        TEXT("[%s] UVisibilityMeshComp::ClearPoolMIDs >> Original materials restored"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()));
}

// -------------------------------------------------------------------------- //
//  UpdateVisibility
// -------------------------------------------------------------------------- //

void UVisibilityMeshComp::UpdateVisibility(float Alpha)
{
    if (VisibilityParam == NAME_None)
        return;

    const float Clamped = FMath::Clamp(Alpha, 0.f, 1.f);

    for (TObjectPtr<UMaterialInstanceDynamic>& MID : MIDs)
        if (IsValid(MID))
            MID->SetScalarParameterValue(VisibilityParam, Clamped);
}
