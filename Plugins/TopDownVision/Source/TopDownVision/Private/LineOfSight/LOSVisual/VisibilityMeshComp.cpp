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
}

// -------------------------------------------------------------------------- //
//  Manual registration
// -------------------------------------------------------------------------- //

void UVisibilityMeshComp::AddMesh(TSoftObjectPtr<USkeletalMeshComponent> Mesh)
{
    if (Mesh.IsNull())
    {
        UE_LOG(VisibilityMeshComp, Warning,
            TEXT("[%s] UVisibilityMeshComp::AddMesh >> Null skeletal soft ptr"),
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
            TEXT("[%s] UVisibilityMeshComp::AddMesh >> Null static soft ptr"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        return;
    }
    StaticMeshTargets.AddUnique(Mesh);
}

// -------------------------------------------------------------------------- //
//  Initialize
// -------------------------------------------------------------------------- //

void UVisibilityMeshComp::Initialize()
{
    if (SkeletalMeshTargets.IsEmpty() && StaticMeshTargets.IsEmpty())
    {
        UE_LOG(VisibilityMeshComp, Warning,
            TEXT("[%s] UVisibilityMeshComp::Initialize >> No mesh targets. Call SetMeshKey() after data loads."),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        return;
    }

    MIDs.Empty();

    auto ProcessMesh = [&](UMeshComponent* Mesh)
    {
        if (!IsValid(Mesh)) return;

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
        if (!SoftMesh.IsValid())
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
        if (!SoftMesh.IsValid())
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
        TEXT("[%s] UVisibilityMeshComp::Initialize >> %d MIDs created"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()), MIDs.Num());
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
        if (IsValid(MID))
            MID->SetScalarParameterValue(VisibilityParam, Clamped);
    }
}
