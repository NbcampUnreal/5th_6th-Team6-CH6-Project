// Fill out your copyright notice in the Description page of Project Settings.


#include "LineOfSight/LOSVisual/VisibilityMeshComp.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInstance.h"
#include "TopDownVisionDebug.h"

DEFINE_LOG_CATEGORY(VisibilityMeshComp);


UVisibilityMeshComp::UVisibilityMeshComp()
{
    PrimaryComponentTick.bCanEverTick = false;
}

// -------------------------------------------------------------------------- //
//  Editor utility
// -------------------------------------------------------------------------- //

void UVisibilityMeshComp::FindMeshesByName()
{
    // This comp is owned by VisionTargetComp which is owned by the actor
    AActor* Actor = GetOwner() ? Cast<AActor>(GetOwner()->GetOwner()) : nullptr;
    if (!Actor)
    {
        UE_LOG(VisibilityMeshComp, Warning,
            TEXT("UVisibilityMeshComp::FindMeshesByName >> Could not resolve actor (owner's owner)"));
        return;
    }

    SkeletalMeshTargets.Empty();
    StaticMeshTargets.Empty();

    // Skeletal meshes
    TArray<USkeletalMeshComponent*> SkelMeshes;
    Actor->GetComponents<USkeletalMeshComponent>(SkelMeshes);
    for (USkeletalMeshComponent* Mesh : SkelMeshes)
    {
        if (!Mesh || !Mesh->ComponentHasTag(VisibilityMeshTag))
            continue;

        SkeletalMeshTargets.Add(TSoftObjectPtr<USkeletalMeshComponent>(Mesh));

        UE_LOG(VisibilityMeshComp, Log,
            TEXT("UVisibilityMeshComp::FindMeshesByName >> Skeletal: %s (%d slots)"),
            *Mesh->GetFName().ToString(), Mesh->GetNumMaterials());
    }

    // Static meshes
    TArray<UStaticMeshComponent*> StaticMeshes;
    Actor->GetComponents<UStaticMeshComponent>(StaticMeshes);
    for (UStaticMeshComponent* Mesh : StaticMeshes)
    {
        if (!Mesh || !Mesh->ComponentHasTag(VisibilityMeshTag))
            continue;

        StaticMeshTargets.Add(TSoftObjectPtr<UStaticMeshComponent>(Mesh));

        UE_LOG(VisibilityMeshComp, Log,
            TEXT("UVisibilityMeshComp::FindMeshesByName >> Static: %s (%d slots)"),
            *Mesh->GetFName().ToString(), Mesh->GetNumMaterials());
    }

    UE_LOG(VisibilityMeshComp, Log,
        TEXT("UVisibilityMeshComp::FindMeshesByName >> Resolved %d skeletal, %d static"),
        SkeletalMeshTargets.Num(), StaticMeshTargets.Num());
}

// -------------------------------------------------------------------------- //
//  Manual registration for meshes outside the actor's direct hierarchy
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

    UE_LOG(VisibilityMeshComp, Log,
        TEXT("[%s] UVisibilityMeshComp::AddMesh >> Added skeletal mesh"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()));
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

    UE_LOG(VisibilityMeshComp, Log,
        TEXT("[%s] UVisibilityMeshComp::AddMesh >> Added static mesh"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()));
}

// -------------------------------------------------------------------------- //
//  Helpers
// -------------------------------------------------------------------------- //

static bool HasParentMaterial(UMaterialInterface* Material, UMaterialInterface* RequiredParent)
{
    if (!Material || !RequiredParent)
        return false;

    UMaterialInterface* Current = Material;
    while (Current)
    {
        if (Current == RequiredParent)
            return true;

        // Walk up — UMaterialInstance has a parent, UMaterial is the root
        UMaterialInstance* AsInstance = Cast<UMaterialInstance>(Current);
        Current = AsInstance ? AsInstance->Parent : nullptr;
    }
    return false;
}

// -------------------------------------------------------------------------- //
//  Initialize — called by VisionTargetComp::BeginPlay
// -------------------------------------------------------------------------- //

void UVisibilityMeshComp::Initialize()
{
    if (SkeletalMeshTargets.IsEmpty() && StaticMeshTargets.IsEmpty())
    {
        UE_LOG(VisibilityMeshComp, Warning,
            TEXT("[%s] UVisibilityMeshComp::Initialize >> MeshTargets empty. Run FindMeshesByName in editor or call AddMesh before Initialize."),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        return;
    }

    MIDs.Empty();

    // Helper lambda to process any mesh component type
    auto ProcessMesh = [&](UMeshComponent* Mesh)
    {
        if (!Mesh)
            return;

        for (int32 i = 0; i < Mesh->GetNumMaterials(); ++i)
        {
            UMaterialInterface* Material = Mesh->GetMaterial(i);
            if (!Material)
                continue;

            // Filter by parent material if set
            if (RequiredParentMaterial && !HasParentMaterial(Material, RequiredParentMaterial))
            {
                UE_LOG(VisibilityMeshComp, Verbose,
                    TEXT("[%s] UVisibilityMeshComp::Initialize >> %s slot[%d] skipped (parent mismatch)"),
                    *TopDownVisionDebug::GetClientDebugName(GetOwner()),
                    *Mesh->GetFName().ToString(), i);
                continue;
            }

            // Also verify the param exists
            float Dummy;
            if (VisibilityParam != NAME_None && !Material->GetScalarParameterValue(VisibilityParam, Dummy))
            {
                UE_LOG(VisibilityMeshComp, Verbose,
                    TEXT("[%s] UVisibilityMeshComp::Initialize >> %s slot[%d] skipped (no param '%s')"),
                    *TopDownVisionDebug::GetClientDebugName(GetOwner()),
                    *Mesh->GetFName().ToString(), i, *VisibilityParam.ToString());
                continue;
            }

            if (UMaterialInstanceDynamic* MID = Mesh->CreateDynamicMaterialInstance(i))
            {
                MIDs.Add(MID);

                UE_LOG(VisibilityMeshComp, Log,
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

    UE_LOG(VisibilityMeshComp, Log,
        TEXT("[%s] UVisibilityMeshComp::Initialize >> %d MIDs created total"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()),
        MIDs.Num());
}

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

    for (UMaterialInstanceDynamic* MID : MIDs)
    {
        if (MID)
            MID->SetScalarParameterValue(VisibilityParam, Clamped);
    }
}
