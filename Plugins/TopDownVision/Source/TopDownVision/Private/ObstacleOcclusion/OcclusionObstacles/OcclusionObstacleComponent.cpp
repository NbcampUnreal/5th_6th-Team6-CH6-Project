// Fill out your copyright notice in the Description page of Project Settings.

#include "ObstacleOcclusion/PhysicallOcclusion/OcclusionObstacleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TopDownVisionDebug.h"


UOcclusionObstacleComponent::UOcclusionObstacleComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UOcclusionObstacleComponent::BeginPlay()
{
    Super::BeginPlay();

    InitializeMaterials();//set MID at runtime
}

void UOcclusionObstacleComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void UOcclusionObstacleComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    const float TargetAlpha = bShouldBeOccluded ? 1.f : 0.f;

    if (bShouldBeOccluded != bLastOcclusionState)
    {
        UE_LOG(Occlusion, Verbose,
            TEXT("UOcclusionObstacleComponent::TickComponent>> Occlusion State Changed -> %s"),
            bShouldBeOccluded ? TEXT("OCCLUDED") : TEXT("VISIBLE"));

        bLastOcclusionState = bShouldBeOccluded;
    }

    CurrentAlpha = FMath::FInterpTo(CurrentAlpha, TargetAlpha, DeltaTime, FadeSpeed);

    UpdateMaterialAlpha();
}

// IOcclusionInterface — called by OcclusionProbeComp hit diff

void UOcclusionObstacleComponent::OnOcclusionEnter_Implementation(UPrimitiveComponent* OccludingComponent)
{
    if (!OccludingComponent)
        return;

    ActiveOverlaps.Add(OccludingComponent);
    bShouldBeOccluded = ActiveOverlaps.Num() > 0;

    UE_LOG(Occlusion, Log,
        TEXT("UOcclusionObstacleComponent::OnOcclusionEnter>> %s | ActiveOverlaps: %d"),
        *OccludingComponent->GetName(),
        ActiveOverlaps.Num());
}

void UOcclusionObstacleComponent::OnOcclusionExit_Implementation(UPrimitiveComponent* OccludingComponent)
{
    if (!OccludingComponent)
        return;

    ActiveOverlaps.Remove(OccludingComponent);
    CleanupInvalidOverlaps();
    bShouldBeOccluded = ActiveOverlaps.Num() > 0;

    UE_LOG(Occlusion, Log,
        TEXT("UOcclusionObstacleComponent::OnOcclusionExit>> %s | ActiveOverlaps: %d"),
        *OccludingComponent->GetName(),
        ActiveOverlaps.Num());
}

// Setup

void UOcclusionObstacleComponent::SetupOcclusionMeshes()
{
    DiscoverChildMeshes();

    UE_LOG(Occlusion, Log,
        TEXT("UOcclusionObstacleComponent::SetupOcclusionMeshes>> Completed setup for %s"),
        *GetOwner()->GetName());
}

void UOcclusionObstacleComponent::DiscoverChildMeshes()
{
    NormalMeshes.Empty();
    OccludedMeshes.Empty();
    NormalDynamicMaterials.Empty();
    OccludedDynamicMaterials.Empty();
    ActiveOverlaps.Empty();

    UE_LOG(Occlusion, Log,
        TEXT("UOcclusionObstacleComponent::DiscoverChildMeshes>> Discovering child meshes for %s"),
        *GetOwner()->GetName());

    TArray<USceneComponent*> Children;
    GetChildrenComponents(true, Children);

    for (USceneComponent* Child : Children)
    {
        if (UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Child))
        {
            if (Mesh->ComponentHasTag(NormalMeshTag))
            {
                NormalMeshes.Add(Mesh);
                UE_LOG(Occlusion, Log,
                    TEXT("UOcclusionObstacleComponent::DiscoverChildMeshes>> NormalMesh: %s"),
                    *Mesh->GetName());
            }
            else if (Mesh->ComponentHasTag(OccludedMeshTag))
            {
                OccludedMeshes.Add(Mesh);
                UE_LOG(Occlusion, Log,
                    TEXT("UOcclusionObstacleComponent::DiscoverChildMeshes>> OccludedMesh: %s"),
                    *Mesh->GetName());
            }
        }
    }

    UE_LOG(Occlusion, Log,
        TEXT("UOcclusionObstacleComponent::DiscoverChildMeshes>> NormalMeshes: %d | OccludedMeshes: %d"),
        NormalMeshes.Num(), OccludedMeshes.Num());

    if (NormalMeshes.Num() == 0 && OccludedMeshes.Num() == 0)
    {
        UE_LOG(Occlusion, Warning,
            TEXT("UOcclusionObstacleComponent::DiscoverChildMeshes>> No tagged meshes found on %s"),
            *GetOwner()->GetName());
    }

    Modify();
}

void UOcclusionObstacleComponent::CleanupInvalidOverlaps()
{
    int32 RemovedCount = 0;

    for (auto It = ActiveOverlaps.CreateIterator(); It; ++It)
    {
        if (!It->IsValid())
        {
            It.RemoveCurrent();
            RemovedCount++;
        }
    }

    if (RemovedCount > 0)
    {
        UE_LOG(Occlusion, Log,
            TEXT("UOcclusionObstacleComponent::CleanupInvalidOverlaps>> Removed %d stale entries"),
            RemovedCount);
    }
}

void UOcclusionObstacleComponent::InitializeMaterials()
{
    UE_LOG(Occlusion, Log,
        TEXT("UOcclusionObstacleComponent::InitializeMaterials>> Creating dynamic materials for %s"),
        *GetOwner()->GetName());

    auto SetupArray =
        [this](const TArray<TSoftObjectPtr<UStaticMeshComponent>>& MeshArray,
               TArray<UMaterialInstanceDynamic*>& OutArray)
        {
            for (TSoftObjectPtr<UStaticMeshComponent> Mesh : MeshArray)
            {
                if (!Mesh) continue;

                const int32 MatCount = Mesh->GetNumMaterials();
                for (int32 i = 0; i < MatCount; ++i)
                {
                    UMaterialInterface* BaseMat = Mesh->GetMaterial(i);
                    if (!BaseMat) continue;

                    UMaterialInstanceDynamic* Dyn =
                        Mesh->CreateDynamicMaterialInstance(i, BaseMat);

                    if (Dyn)
                    {
                        OutArray.Add(Dyn);
                        UE_LOG(Occlusion, Log,
                            TEXT("UOcclusionObstacleComponent::InitializeMaterials>> Created DynMat on %s (Index %d)"),
                            *Mesh->GetName(), i);
                    }
                }
            }
        };

    SetupArray(NormalMeshes,   NormalDynamicMaterials);
    SetupArray(OccludedMeshes, OccludedDynamicMaterials);

    UE_LOG(Occlusion, Log,
        TEXT("UOcclusionObstacleComponent::InitializeMaterials>> NormalDyn: %d | OccludedDyn: %d"),
        NormalDynamicMaterials.Num(), OccludedDynamicMaterials.Num());
}

void UOcclusionObstacleComponent::UpdateMaterialAlpha()
{
    const float NormalAlpha   = 1.f - CurrentAlpha;
    const float OccludedAlpha = CurrentAlpha;

    for (UMaterialInstanceDynamic* Dyn : NormalDynamicMaterials)
    {
        if (Dyn) Dyn->SetScalarParameterValue(AlphaParameterName, NormalAlpha);
    }

    for (UMaterialInstanceDynamic* Dyn : OccludedDynamicMaterials)
    {
        if (Dyn) Dyn->SetScalarParameterValue(AlphaParameterName, OccludedAlpha);
    }
}
