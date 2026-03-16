#include "ObstacleOcclusion/Helper/OcclusionMeshUtil.h"
#include "Components/MeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

DEFINE_LOG_CATEGORY(OcclusionMeshHelper);

// ── Discover ──────────────────────────────────────────────────────────────────

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
            TEXT("UOcclusionMeshUtil::DiscoverChildMeshes>> Root is null"));
        return;
    }

    UE_LOG(OcclusionMeshHelper, Log,
        TEXT("UOcclusionMeshUtil::DiscoverChildMeshes>> Scanning children of %s"),
        *Root->GetOwner()->GetName());

    TArray<USceneComponent*> Children;
    Root->GetChildrenComponents(true, Children);

    for (USceneComponent* Child : Children)
    {
        UMeshComponent* Mesh = Cast<UMeshComponent>(Child);
        if (!Mesh) continue;

        if (NormalTag != NAME_None && Mesh->ComponentHasTag(NormalTag))
        {
            OutNormalMeshes.Add(Mesh);
            UE_LOG(OcclusionMeshHelper, Log,
                TEXT("UOcclusionMeshUtil::DiscoverChildMeshes>> NormalMesh: %s (%s)"),
                *Mesh->GetName(), *Mesh->GetClass()->GetName());
        }
        else if (OccludedTag != NAME_None && Mesh->ComponentHasTag(OccludedTag))
        {
            OutOccludedMeshes.Add(Mesh);
            UE_LOG(OcclusionMeshHelper, Log,
                TEXT("UOcclusionMeshUtil::DiscoverChildMeshes>> OccludedMesh: %s (%s)"),
                *Mesh->GetName(), *Mesh->GetClass()->GetName());
        }
    }

    UE_LOG(OcclusionMeshHelper, Log,
        TEXT("UOcclusionMeshUtil::DiscoverChildMeshes>> Normal: %d | Occluded: %d"),
        OutNormalMeshes.Num(), OutOccludedMeshes.Num());

    if (OutNormalMeshes.Num() == 0 && OutOccludedMeshes.Num() == 0)
    {
        UE_LOG(OcclusionMeshHelper, Warning,
            TEXT("UOcclusionMeshUtil::DiscoverChildMeshes>> No tagged meshes found on %s"),
            *Root->GetOwner()->GetName());
    }
}

// ── MID Creation — internal helper ───────────────────────────────────────────

static void CreateMIDsForMeshType(
    const TArray<TSoftObjectPtr<UMeshComponent>>& Meshes,
    TArray<UMaterialInstanceDynamic*>& OutMIDs,
    bool bStaticOnly)
{
    OutMIDs.Empty();

    for (const TSoftObjectPtr<UMeshComponent>& MeshPtr : Meshes)
    {
        UMeshComponent* Mesh = MeshPtr.Get();
        if (!Mesh) continue;

        const bool bIsStatic   = Mesh->IsA<UStaticMeshComponent>();
        const bool bIsSkeletal = Mesh->IsA<USkeletalMeshComponent>();

        if (bStaticOnly  && !bIsStatic)  continue;
        if (!bStaticOnly && !bIsSkeletal) continue;

        const int32 MaterialCount = Mesh->GetNumMaterials();

        UE_LOG(OcclusionMeshHelper, Log,
            TEXT("UOcclusionMeshUtil::CreateMIDsForMeshType>> %s has %d material slots [%s]"),
            *Mesh->GetName(), MaterialCount, bStaticOnly ? TEXT("Static") : TEXT("Skeletal"));

        for (int32 i = 0; i < MaterialCount; i++)
        {
            UMaterialInterface* BaseMaterial = Mesh->GetMaterial(i);
            if (!BaseMaterial)
            {
                UE_LOG(OcclusionMeshHelper, Warning,
                    TEXT("UOcclusionMeshUtil::CreateMIDsForMeshType>> %s slot %d has no material — skipping"),
                    *Mesh->GetName(), i);
                continue;
            }

            UMaterialInstanceDynamic* MID = Mesh->CreateDynamicMaterialInstance(i, BaseMaterial);
            if (MID)
            {
                OutMIDs.Add(MID);
                UE_LOG(OcclusionMeshHelper, Log,
                    TEXT("UOcclusionMeshUtil::CreateMIDsForMeshType>> MID created — %s slot %d"),
                    *Mesh->GetName(), i);
            }
            else
            {
                UE_LOG(OcclusionMeshHelper, Warning,
                    TEXT("UOcclusionMeshUtil::CreateMIDsForMeshType>> Failed MID — %s slot %d"),
                    *Mesh->GetName(), i);
            }
        }
    }

    UE_LOG(OcclusionMeshHelper, Log,
        TEXT("UOcclusionMeshUtil::CreateMIDsForMeshType>> Total MIDs [%s]: %d"),
        bStaticOnly ? TEXT("Static") : TEXT("Skeletal"), OutMIDs.Num());
}

void UOcclusionMeshUtil::CreateDynamicMaterials_Static(
    const TArray<TSoftObjectPtr<UMeshComponent>>& Meshes,
    TArray<UMaterialInstanceDynamic*>& OutMIDs)
{
    CreateMIDsForMeshType(Meshes, OutMIDs, true);
}

void UOcclusionMeshUtil::CreateDynamicMaterials_Skeletal(
    const TArray<TSoftObjectPtr<UMeshComponent>>& Meshes,
    TArray<UMaterialInstanceDynamic*>& OutMIDs)
{
    CreateMIDsForMeshType(Meshes, OutMIDs, false);
}

// ── Shadow Proxy Generation ───────────────────────────────────────────────────

void UOcclusionMeshUtil::GenerateShadowProxyMeshes(
    AActor* Owner,
    const TArray<TSoftObjectPtr<UMeshComponent>>& SourceMeshes,
    UMaterialInterface* ShadowProxyMaterial,
    TArray<TObjectPtr<UStaticMeshComponent>>& OutStaticProxies,
    TArray<TObjectPtr<USkeletalMeshComponent>>& OutSkeletalProxies)
{
    if (!Owner)
    {
        UE_LOG(OcclusionMeshHelper, Warning,
            TEXT("UOcclusionMeshUtil::GenerateShadowProxyMeshes>> Owner is null"));
        return;
    }

    // Destroy existing proxies
    for (TObjectPtr<UStaticMeshComponent> Proxy : OutStaticProxies)
        if (Proxy) Proxy->DestroyComponent();
    OutStaticProxies.Empty();

    for (TObjectPtr<USkeletalMeshComponent> Proxy : OutSkeletalProxies)
        if (Proxy) Proxy->DestroyComponent();
    OutSkeletalProxies.Empty();

    if (!IsValid(ShadowProxyMaterial))
    {
        UE_LOG(OcclusionMeshHelper, Warning,
            TEXT("UOcclusionMeshUtil::GenerateShadowProxyMeshes>> No ShadowProxyMaterial set on %s — proxies will cast no shadow"),
            *Owner->GetName());
    }

    for (const TSoftObjectPtr<UMeshComponent>& MeshPtr : SourceMeshes)
    {
        UMeshComponent* SourceMesh = MeshPtr.Get();
        if (!SourceMesh) continue;

        // ── Static proxy ──────────────────────────────────────────────────
        if (UStaticMeshComponent* SourceStatic = Cast<UStaticMeshComponent>(SourceMesh))
        {
            UStaticMeshComponent* Proxy = NewObject<UStaticMeshComponent>(
                Owner, UStaticMeshComponent::StaticClass(), NAME_None, RF_NoFlags);
            if (!Proxy) continue;

            Proxy->SetStaticMesh(SourceStatic->GetStaticMesh());
            Proxy->AttachToComponent(
                SourceStatic,
                FAttachmentTransformRules::SnapToTargetIncludingScale);

            Proxy->SetVisibility(false);
            Proxy->SetHiddenInGame(true);
            Proxy->SetCastShadow(true);
            Proxy->bCastHiddenShadow  = true;
            Proxy->bCastDynamicShadow = true;
            Proxy->SetCollisionEnabled(ECollisionEnabled::NoCollision);

            if (IsValid(ShadowProxyMaterial))
                for (int32 i = 0; i < Proxy->GetNumMaterials(); i++)
                    Proxy->SetMaterial(i, ShadowProxyMaterial);

            Proxy->RegisterComponent();
            Owner->AddInstanceComponent(Proxy);
            OutStaticProxies.Add(Proxy);

            UE_LOG(OcclusionMeshHelper, Log,
                TEXT("UOcclusionMeshUtil::GenerateShadowProxyMeshes>> Static proxy created for %s"),
                *SourceStatic->GetName());
        }
        // ── Skeletal proxy ────────────────────────────────────────────────
        else if (USkeletalMeshComponent* SourceSkeletal = Cast<USkeletalMeshComponent>(SourceMesh))
        {
            USkeletalMeshComponent* Proxy = NewObject<USkeletalMeshComponent>(
                Owner, USkeletalMeshComponent::StaticClass(), NAME_None, RF_NoFlags);
            if (!Proxy) continue;

            Proxy->SetSkeletalMeshAsset(SourceSkeletal->GetSkeletalMeshAsset());
            Proxy->SetLeaderPoseComponent(SourceSkeletal);
            Proxy->AttachToComponent(
                SourceSkeletal,
                FAttachmentTransformRules::SnapToTargetIncludingScale);

            Proxy->SetVisibility(false);
            Proxy->SetHiddenInGame(true);
            Proxy->SetCastShadow(true);
            Proxy->bCastHiddenShadow  = true;
            Proxy->bCastDynamicShadow = true;
            Proxy->SetCollisionEnabled(ECollisionEnabled::NoCollision);

            if (IsValid(ShadowProxyMaterial))
                for (int32 i = 0; i < Proxy->GetNumMaterials(); i++)
                    Proxy->SetMaterial(i, ShadowProxyMaterial);

            Proxy->RegisterComponent();
            Owner->AddInstanceComponent(Proxy);
            OutSkeletalProxies.Add(Proxy);

            UE_LOG(OcclusionMeshHelper, Log,
                TEXT("UOcclusionMeshUtil::GenerateShadowProxyMeshes>> Skeletal proxy created for %s"),
                *SourceSkeletal->GetName());
        }
    }

    UE_LOG(OcclusionMeshHelper, Log,
        TEXT("UOcclusionMeshUtil::GenerateShadowProxyMeshes>> Done for %s — Static: %d | Skeletal: %d"),
        *Owner->GetName(), OutStaticProxies.Num(), OutSkeletalProxies.Num());
}