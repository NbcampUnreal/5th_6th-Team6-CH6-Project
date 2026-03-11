#include "ObstacleOcclusion/Helper/OcclusionMeshUtil.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

DEFINE_LOG_CATEGORY(OcclusionMeshHelper);

void UOcclusionMeshUtil::DiscoverChildMeshes(
    USceneComponent* Root,
    FName NormalTag,
    FName OccludedTag,
    TArray<TSoftObjectPtr<UStaticMeshComponent>>& OutNormalMeshes,
    TArray<TSoftObjectPtr<UStaticMeshComponent>>& OutOccludedMeshes)
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
        if (UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Child))
        {
            if (Mesh->ComponentHasTag(NormalTag))
            {
                OutNormalMeshes.Add(Mesh);

                UE_LOG(OcclusionMeshHelper, Log,
                    TEXT("UOcclusionMeshUtil::DiscoverChildMeshes>> Found NormalMesh: %s"),
                    *Mesh->GetName());
            }
            else if (Mesh->ComponentHasTag(OccludedTag))
            {
                OutOccludedMeshes.Add(Mesh);

                UE_LOG(OcclusionMeshHelper, Log,
                    TEXT("UOcclusionMeshUtil::DiscoverChildMeshes>> Found OccludedMesh: %s"),
                    *Mesh->GetName());
            }
        }
    }

    UE_LOG(OcclusionMeshHelper, Log,
        TEXT("UOcclusionMeshUtil::DiscoverChildMeshes>> Result -> Normal: %d | Occluded: %d"),
        OutNormalMeshes.Num(),
        OutOccludedMeshes.Num());

    if (OutNormalMeshes.Num() == 0 && OutOccludedMeshes.Num() == 0)
    {
        UE_LOG(OcclusionMeshHelper, Warning,
            TEXT("UOcclusionMeshUtil::DiscoverChildMeshes>> No tagged meshes found on actor: %s"),
            *Root->GetOwner()->GetName());
    }
}

void UOcclusionMeshUtil::CreateDynamicMaterials(const TArray<TSoftObjectPtr<UStaticMeshComponent>>& Meshes,
    TArray<UMaterialInstanceDynamic*>& OutMIDs)
{
    OutMIDs.Empty();

    UE_LOG(OcclusionMeshHelper, Log,
        TEXT("UOcclusionMeshUtil::CreateDynamicMaterials>> Creating MIDs for %d meshes"),
        Meshes.Num());

    for (const TSoftObjectPtr<UStaticMeshComponent>& MeshPtr : Meshes)
    {
        UStaticMeshComponent* Mesh = MeshPtr.Get();

        if (!Mesh)
        {
            UE_LOG(OcclusionMeshHelper, Warning,
                TEXT("UOcclusionMeshUtil::CreateDynamicMaterials>> MeshPtr invalid"));
            continue;
        }

        int32 MaterialCount = Mesh->GetNumMaterials();

        UE_LOG(OcclusionMeshHelper, Log,
            TEXT("UOcclusionMeshUtil::CreateDynamicMaterials>> Mesh %s has %d materials"),
            *Mesh->GetName(),
            MaterialCount);

        for (int32 i = 0; i < MaterialCount; i++)
        {
            UMaterialInterface* BaseMaterial = Mesh->GetMaterial(i);

            if (!BaseMaterial)
            {
                UE_LOG(OcclusionMeshHelper, Warning,
                    TEXT("UOcclusionMeshUtil::CreateDynamicMaterials>> Mesh %s slot %d has no material"),
                    *Mesh->GetName(),
                    i);
                continue;
            }

            UMaterialInstanceDynamic* MID =
                Mesh->CreateDynamicMaterialInstance(i, BaseMaterial);

            if (MID)
            {
                OutMIDs.Add(MID);

                UE_LOG(OcclusionMeshHelper, Log,
                    TEXT("UOcclusionMeshUtil::CreateDynamicMaterials>> MID created for %s slot %d"),
                    *Mesh->GetName(),
                    i);
            }
            else
            {
                UE_LOG(OcclusionMeshHelper, Warning,
                    TEXT("UOcclusionMeshUtil::CreateDynamicMaterials>> Failed MID creation for %s slot %d"),
                    *Mesh->GetName(),
                    i);
            }
        }
    }

    UE_LOG(OcclusionMeshHelper, Log,
        TEXT("UOcclusionMeshUtil::CreateDynamicMaterials>> Total MIDs created: %d"),
        OutMIDs.Num());
}
