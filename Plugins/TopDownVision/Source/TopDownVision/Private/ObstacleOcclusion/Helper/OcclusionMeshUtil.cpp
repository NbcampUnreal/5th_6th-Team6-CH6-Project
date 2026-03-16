#include "ObstacleOcclusion/Helper/OcclusionMeshUtil.h"
#include "Components/MeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

DEFINE_LOG_CATEGORY(OcclusionMeshHelper);

void UOcclusionMeshUtil::DiscoverChildMeshes(
    USceneComponent* Root,
    FName NormalTag,
    FName OccludedTag,
    TArray<TSoftObjectPtr<UMeshComponent>>& OutNormalMeshes,
    TArray<TSoftObjectPtr<UMeshComponent>>& OutOccludedMeshes)
{
    OutNormalMeshes.Empty();
    OutOccludedMeshes.Empty();

    if (!Root)
    {
        UE_LOG(OcclusionMeshHelper, Warning,
            TEXT("UOcclusionMeshUtil::DiscoverChildMeshes>> Root component is null"));
        return;
    }

    UE_LOG(OcclusionMeshHelper, Log,
        TEXT("UOcclusionMeshUtil::DiscoverChildMeshes>> Scanning children of %s"),
        *Root->GetOwner()->GetName());

    TArray<USceneComponent*> Children;
    Root->GetChildrenComponents(true, Children);

    for (USceneComponent* Child : Children)
    {
        // UMeshComponent is common parent of UStaticMeshComponent and USkeletalMeshComponent
        UMeshComponent* Mesh = Cast<UMeshComponent>(Child);
        if (!Mesh) continue;

        if (NormalTag != NAME_None && Mesh->ComponentHasTag(NormalTag))
        {
            OutNormalMeshes.Add(Mesh);

            UE_LOG(OcclusionMeshHelper, Log,
                TEXT("UOcclusionMeshUtil::DiscoverChildMeshes>> Found NormalMesh: %s (%s)"),
                *Mesh->GetName(), *Mesh->GetClass()->GetName());
        }
        else if (OccludedTag != NAME_None && Mesh->ComponentHasTag(OccludedTag))
        {
            OutOccludedMeshes.Add(Mesh);

            UE_LOG(OcclusionMeshHelper, Log,
                TEXT("UOcclusionMeshUtil::DiscoverChildMeshes>> Found OccludedMesh: %s (%s)"),
                *Mesh->GetName(), *Mesh->GetClass()->GetName());
        }
    }

    UE_LOG(OcclusionMeshHelper, Log,
        TEXT("UOcclusionMeshUtil::DiscoverChildMeshes>> Result -> Normal: %d | Occluded: %d"),
        OutNormalMeshes.Num(), OutOccludedMeshes.Num());

    if (OutNormalMeshes.Num() == 0 && OutOccludedMeshes.Num() == 0)
    {
        UE_LOG(OcclusionMeshHelper, Warning,
            TEXT("UOcclusionMeshUtil::DiscoverChildMeshes>> No tagged meshes found on actor: %s"),
            *Root->GetOwner()->GetName());
    }
}

void UOcclusionMeshUtil::CreateDynamicMaterials(
    const TArray<TSoftObjectPtr<UMeshComponent>>& Meshes,
    TArray<UMaterialInstanceDynamic*>& OutMIDs)
{
    OutMIDs.Empty();

    UE_LOG(OcclusionMeshHelper, Log,
        TEXT("UOcclusionMeshUtil::CreateDynamicMaterials>> Creating MIDs for %d meshes"),
        Meshes.Num());

    for (const TSoftObjectPtr<UMeshComponent>& MeshPtr : Meshes)
    {
        UMeshComponent* Mesh = MeshPtr.Get();

        if (!Mesh)
        {
            UE_LOG(OcclusionMeshHelper, Warning,
                TEXT("UOcclusionMeshUtil::CreateDynamicMaterials>> MeshPtr invalid"));
            continue;
        }

        const int32 MaterialCount = Mesh->GetNumMaterials();

        UE_LOG(OcclusionMeshHelper, Log,
            TEXT("UOcclusionMeshUtil::CreateDynamicMaterials>> Mesh %s (%s) has %d materials"),
            *Mesh->GetName(), *Mesh->GetClass()->GetName(), MaterialCount);

        for (int32 i = 0; i < MaterialCount; i++)
        {
            UMaterialInterface* BaseMaterial = Mesh->GetMaterial(i);

            if (!BaseMaterial)
            {
                UE_LOG(OcclusionMeshHelper, Warning,
                    TEXT("UOcclusionMeshUtil::CreateDynamicMaterials>> Mesh %s slot %d has no material"),
                    *Mesh->GetName(), i);
                continue;
            }

            UMaterialInstanceDynamic* MID =
                Mesh->CreateDynamicMaterialInstance(i, BaseMaterial);

            if (MID)
            {
                OutMIDs.Add(MID);

                UE_LOG(OcclusionMeshHelper, Log,
                    TEXT("UOcclusionMeshUtil::CreateDynamicMaterials>> MID created for %s slot %d"),
                    *Mesh->GetName(), i);
            }
            else
            {
                UE_LOG(OcclusionMeshHelper, Warning,
                    TEXT("UOcclusionMeshUtil::CreateDynamicMaterials>> Failed MID creation for %s slot %d"),
                    *Mesh->GetName(), i);
            }
        }
    }

    UE_LOG(OcclusionMeshHelper, Log,
        TEXT("UOcclusionMeshUtil::CreateDynamicMaterials>> Total MIDs created: %d"),
        OutMIDs.Num());
}