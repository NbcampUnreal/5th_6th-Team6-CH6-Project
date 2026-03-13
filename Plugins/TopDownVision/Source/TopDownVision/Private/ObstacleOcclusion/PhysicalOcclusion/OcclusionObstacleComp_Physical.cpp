#include "TopDownVision/Public/ObstacleOcclusion/PhysicalOcclusion/OcclusionObstacleComp_Physical.h"
#include "Components/MeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ObstacleOcclusion/Helper/OcclusionMeshUtil.h"
#include "TopDownVisionDebug.h"

UOcclusionObstacleComp_Physical::UOcclusionObstacleComp_Physical()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UOcclusionObstacleComp_Physical::BeginPlay()
{
    Super::BeginPlay();

    InitializeMaterials(); // set MID at runtime
}

void UOcclusionObstacleComp_Physical::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void UOcclusionObstacleComp_Physical::TickComponent(float DeltaTime, ELevelTick TickType,
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

// ── IOcclusionInterface — called by OcclusionProbeComp hit diff ───────────────

void UOcclusionObstacleComp_Physical::OnOcclusionEnter_Implementation(UObject* SourceTracer)
{
    if (!SourceTracer) return;

    ActiveOverlaps.Add(SourceTracer);
    bShouldBeOccluded = ActiveOverlaps.Num() > 0;

    UE_LOG(Occlusion, Log,
        TEXT("UOcclusionObstacleComponent::OnOcclusionEnter>> %s | ActiveOverlaps: %d"),
        *SourceTracer->GetName(), ActiveOverlaps.Num());
}

void UOcclusionObstacleComp_Physical::OnOcclusionExit_Implementation(UObject* SourceTracer)
{
    if (!SourceTracer) return;

    ActiveOverlaps.Remove(SourceTracer);
    CleanupInvalidOverlaps();
    bShouldBeOccluded = ActiveOverlaps.Num() > 0;

    UE_LOG(Occlusion, Log,
        TEXT("UOcclusionObstacleComponent::OnOcclusionExit>> %s | ActiveOverlaps: %d"),
        *SourceTracer->GetName(), ActiveOverlaps.Num());
}

// ── Setup ─────────────────────────────────────────────────────────────────────

void UOcclusionObstacleComp_Physical::SetupOcclusionMeshes()
{
    DiscoverChildMeshes();
    GenerateShadowProxyMeshes();

    UE_LOG(Occlusion, Log,
        TEXT("UOcclusionObstacleComponent::SetupOcclusionMeshes>> Completed setup for %s"),
        *GetOwner()->GetName());
}

void UOcclusionObstacleComp_Physical::InitializeCollisionAndShadow()
{
    for (TSoftObjectPtr<UMeshComponent> MeshPtr : NormalMeshes)
    {
        UMeshComponent* Mesh = MeshPtr.Get();
        if (!Mesh) continue;

        // Collision only on static meshes — skeletal uses physics asset
        if (UStaticMeshComponent* StaticMesh = Cast<UStaticMeshComponent>(Mesh))
        {
            StaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            StaticMesh->SetCollisionResponseToChannel(OcclusionTraceChannel, ECR_Block);
            StaticMesh->SetCollisionResponseToChannel(MouseTraceChannel, ECR_Ignore);

            UE_LOG(Occlusion, Log,
                TEXT("UOcclusionObstacleComponent::InitializeCollisionAndShadow>> Set ECR_Block on %s"),
                *StaticMesh->GetName());
        }

        // Shadow proxy handles shadow — disable on original mesh
        Mesh->SetCastShadow(false);
    }

    for (TSoftObjectPtr<UMeshComponent> MeshPtr : OccludedMeshes)
    {
        UMeshComponent* Mesh = MeshPtr.Get();
        if (!Mesh) continue;

        if (UStaticMeshComponent* StaticMesh = Cast<UStaticMeshComponent>(Mesh))
        {
            StaticMesh->SetCollisionResponseToChannel(MouseTraceChannel, ECR_Ignore);
        }

        Mesh->SetCastShadow(false);
    }
}

void UOcclusionObstacleComp_Physical::GenerateShadowProxyMeshes()
{
    // Destroy existing static proxies
    for (TObjectPtr<UStaticMeshComponent> Proxy : StaticShadowProxies)
    {
        if (Proxy) Proxy->DestroyComponent();
    }
    StaticShadowProxies.Empty();

    // Destroy existing skeletal proxies
    for (TObjectPtr<USkeletalMeshComponent> Proxy : SkeletalShadowProxies)
    {
        if (Proxy) Proxy->DestroyComponent();
    }
    SkeletalShadowProxies.Empty();

    if (!IsValid(ShadowProxyMaterial))
    {
        UE_LOG(Occlusion, Warning,
            TEXT("UOcclusionObstacleComponent::GenerateShadowProxyMeshes>> No ShadowProxyMaterial set on %s — proxies will have no material"),
            *GetOwner()->GetName());
    }

    for (TSoftObjectPtr<UMeshComponent> MeshPtr : NormalMeshes)
    {
        UMeshComponent* SourceMesh = MeshPtr.Get();
        if (!SourceMesh) continue;

        // ── Static mesh proxy ─────────────────────────────────────────────
        if (UStaticMeshComponent* SourceStatic = Cast<UStaticMeshComponent>(SourceMesh))
        {
            UStaticMeshComponent* Proxy = NewObject<UStaticMeshComponent>(
                GetOwner(),
                UStaticMeshComponent::StaticClass(),
                NAME_None,
                RF_NoFlags);

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

            // Curved world proxy material — WPO driven by MPC, no MID needed
            if (IsValid(ShadowProxyMaterial))
            {
                for (int32 i = 0; i < Proxy->GetNumMaterials(); i++)
                {
                    Proxy->SetMaterial(i, ShadowProxyMaterial);
                }
            }

            Proxy->RegisterComponent();
            GetOwner()->AddInstanceComponent(Proxy); // serialized with level

            StaticShadowProxies.Add(Proxy);

            UE_LOG(Occlusion, Log,
                TEXT("UOcclusionObstacleComponent::GenerateShadowProxyMeshes>> Created static proxy for %s"),
                *SourceStatic->GetName());
        }
        // ── Skeletal mesh proxy ───────────────────────────────────────────
        else if (USkeletalMeshComponent* SourceSkeletal = Cast<USkeletalMeshComponent>(SourceMesh))
        {
            USkeletalMeshComponent* Proxy = NewObject<USkeletalMeshComponent>(
                GetOwner(),
                USkeletalMeshComponent::StaticClass(),
                NAME_None,
                RF_NoFlags);

            if (!Proxy) continue;

            Proxy->SetSkeletalMeshAsset(SourceSkeletal->GetSkeletalMeshAsset());

            // Mirrors all bone transforms from source automatically — no manual anim sync needed
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

            // Curved world proxy material — fully opaque so shadow never fades with mesh opacity
            // WPO driven by MPC — no MID needed
            if (IsValid(ShadowProxyMaterial))
            {
                for (int32 i = 0; i < Proxy->GetNumMaterials(); i++)
                {
                    Proxy->SetMaterial(i, ShadowProxyMaterial);
                }
            }

            Proxy->RegisterComponent();
            GetOwner()->AddInstanceComponent(Proxy); // serialized with level

            SkeletalShadowProxies.Add(Proxy);

            UE_LOG(Occlusion, Log,
                TEXT("UOcclusionObstacleComponent::GenerateShadowProxyMeshes>> Created skeletal proxy for %s"),
                *SourceSkeletal->GetName());
        }
    }

    Modify();
    MarkPackageDirty();

    UE_LOG(Occlusion, Log,
        TEXT("UOcclusionObstacleComponent::GenerateShadowProxyMeshes>> Static proxies: %d | Skeletal proxies: %d"),
        StaticShadowProxies.Num(), SkeletalShadowProxies.Num());
}

void UOcclusionObstacleComp_Physical::DiscoverChildMeshes()
{
    NormalMeshes.Empty();
    OccludedMeshes.Empty();
    NormalDynamicMaterials.Empty();
    OccludedDynamicMaterials.Empty();
    ActiveOverlaps.Empty();

    UE_LOG(Occlusion, Log,
        TEXT("UOcclusionObstacleComponent::DiscoverChildMeshes>> Discovering child meshes for %s"),
        *GetOwner()->GetName());

    UOcclusionMeshUtil::DiscoverChildMeshes(
        this,
        NormalMeshTag,
        OccludedMeshTag,
        NormalMeshes,
        OccludedMeshes);

    UE_LOG(Occlusion, Log,
        TEXT("UOcclusionObstacleComponent::DiscoverChildMeshes>> NormalMeshes: %d | OccludedMeshes: %d"),
        NormalMeshes.Num(), OccludedMeshes.Num());

    Modify(); // mark dirty
}

void UOcclusionObstacleComp_Physical::CleanupInvalidOverlaps()
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

void UOcclusionObstacleComp_Physical::InitializeMaterials()
{
    UE_LOG(Occlusion, Log,
        TEXT("UOcclusionObstacleComponent::InitializeMaterials>> Creating dynamic materials for %s"),
        *GetOwner()->GetName());

    UOcclusionMeshUtil::CreateDynamicMaterials(NormalMeshes, NormalDynamicMaterials);
    UOcclusionMeshUtil::CreateDynamicMaterials(OccludedMeshes, OccludedDynamicMaterials);

    UE_LOG(Occlusion, Log,
        TEXT("UOcclusionObstacleComponent::InitializeMaterials>> NormalDyn: %d | OccludedDyn: %d"),
        NormalDynamicMaterials.Num(), OccludedDynamicMaterials.Num());
}

void UOcclusionObstacleComp_Physical::UpdateMaterialAlpha()
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