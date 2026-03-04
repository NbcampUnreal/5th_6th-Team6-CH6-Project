// Fill out your copyright notice in the Description page of Project Settings.

#include "LineOfSight/LOSVisual/VisibilityMeshComp.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
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
    /*// This comp is owned by VisionTargetComp which is owned by the actor
    AActor* Actor = GetOwner() ? Cast<AActor>(GetOwner()->GetOwner()) : nullptr;
    if (!Actor)
    {
        UE_LOG(VisibilityMeshComp, Warning,
            TEXT("UVisibilityMeshComp::FindMeshesByTag >> Could not resolve actor (owner's owner)"));
        return;
    }*/

    AActor* Actor = GetOwner();
    if (!Actor)
    {
        UE_LOG(VisibilityMeshComp, Warning,
            TEXT("UVisibilityMeshComp::FindMeshesByTag >> No owning actor"));
        return;
    }

    //reset
    SkeletalMeshTargets.Empty();
    StaticMeshTargets.Empty();

    TArray<USkeletalMeshComponent*> SkelMeshes;
    Actor->GetComponents<USkeletalMeshComponent>(SkelMeshes);
    for (USkeletalMeshComponent* Mesh : SkelMeshes)
    {
        if (!Mesh || !Mesh->ComponentHasTag(VisibilityMeshTag))
            continue;

        SkeletalMeshTargets.Add(TSoftObjectPtr<USkeletalMeshComponent>(Mesh));

        UE_LOG(VisibilityMeshComp, Log,
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

        UE_LOG(VisibilityMeshComp, Log,
            TEXT("UVisibilityMeshComp::FindMeshesByName >> Static: %s (%d slots)"),
            *Mesh->GetFName().ToString(), Mesh->GetNumMaterials());
    }

    UE_LOG(VisibilityMeshComp, Log,
        TEXT("UVisibilityMeshComp::FindMeshesByName >> Resolved %d skeletal, %d static"),
        SkeletalMeshTargets.Num(), StaticMeshTargets.Num());
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

    auto ProcessMesh = [&](UMeshComponent* Mesh)
    {
        if (!Mesh)
            return;

        for (int32 i = 0; i < Mesh->GetNumMaterials(); ++i)
        {
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

    //now set the visibility as 0 for default
    
    UpdateVisibility(0.f);

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
