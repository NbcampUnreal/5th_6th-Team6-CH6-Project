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
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UOcclusionObstacleComp_Physical::BeginPlay()
{
    Super::BeginPlay();
    InitializeMaterials();
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
            TEXT("UOcclusionObstacleComp_Physical::TickComponent>> State -> %s"),
            bShouldBeOccluded ? TEXT("OCCLUDED") : TEXT("VISIBLE"));
        bLastOcclusionState = bShouldBeOccluded;
    }

    CurrentAlpha = FMath::FInterpTo(CurrentAlpha, TargetAlpha, DeltaTime, FadeSpeed);
    UpdateMaterialAlpha();

    if (FMath::IsNearlyEqual(CurrentAlpha, TargetAlpha, 0.001f))
    {
        CurrentAlpha = TargetAlpha;
        UpdateMaterialAlpha();
        SetComponentTickEnabled(false);
    }
}

// ── IOcclusionInterface ───────────────────────────────────────────────────────

void UOcclusionObstacleComp_Physical::OnOcclusionEnter_Implementation(UObject* SourceTracer)
{
    if (!SourceTracer) return;
    ActiveOverlaps.Add(SourceTracer);
    bShouldBeOccluded = ActiveOverlaps.Num() > 0;
    SetComponentTickEnabled(true);

    UE_LOG(Occlusion, Log,
        TEXT("UOcclusionObstacleComp_Physical::OnOcclusionEnter>> %s | ActiveOverlaps: %d"),
        *SourceTracer->GetName(), ActiveOverlaps.Num());
}

void UOcclusionObstacleComp_Physical::OnOcclusionExit_Implementation(UObject* SourceTracer)
{
    if (!SourceTracer) return;
    ActiveOverlaps.Remove(SourceTracer);
    CleanupInvalidOverlaps();
    bShouldBeOccluded = ActiveOverlaps.Num() > 0;
    SetComponentTickEnabled(true);

    UE_LOG(Occlusion, Log,
        TEXT("UOcclusionObstacleComp_Physical::OnOcclusionExit>> %s | ActiveOverlaps: %d"),
        *SourceTracer->GetName(), ActiveOverlaps.Num());
}

// ── Setup ─────────────────────────────────────────────────────────────────────

void UOcclusionObstacleComp_Physical::SetupOcclusionMeshes()
{
    DiscoverChildMeshes();
    
    if (bCastShadowWhenOccluded)
    {
        GenerateShadowProxyMeshes();
    }
    else 
    {
        // Destroy any proxies left over from a previous setup run
        for (TObjectPtr<UStaticMeshComponent> Proxy : StaticShadowProxies)
            if (Proxy) Proxy->DestroyComponent();
        StaticShadowProxies.Empty();

        for (TObjectPtr<USkeletalMeshComponent> Proxy : SkeletalShadowProxies)
            if (Proxy) Proxy->DestroyComponent();
        SkeletalShadowProxies.Empty();

        UE_LOG(Occlusion, Log,
            TEXT("UOcclusionObstacleComp_Physical::SetupOcclusionMeshes>> Shadow proxies removed for %s"),
            *GetOwner()->GetName());
    }


    UE_LOG(Occlusion, Log,
        TEXT("UOcclusionObstacleComp_Physical::SetupOcclusionMeshes>> Completed for %s"),
        *GetOwner()->GetName());
}

void UOcclusionObstacleComp_Physical::InitializeCollisionAndShadow()
{
    for (TSoftObjectPtr<UMeshComponent> MeshPtr : NormalMeshes)
    {
        UMeshComponent* Mesh = MeshPtr.Get();
        if (!Mesh) continue;

        if (UStaticMeshComponent* StaticMesh = Cast<UStaticMeshComponent>(Mesh))
        {
            StaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            StaticMesh->SetCollisionResponseToChannel(OcclusionTraceChannel, ECR_Block);
            StaticMesh->SetCollisionResponseToChannel(MouseTraceChannel, ECR_Ignore);
        }

        Mesh->SetCastShadow(false);
    }

    for (TSoftObjectPtr<UMeshComponent> MeshPtr : OccludedMeshes)
    {
        UMeshComponent* Mesh = MeshPtr.Get();
        if (!Mesh) continue;

        if (UStaticMeshComponent* StaticMesh = Cast<UStaticMeshComponent>(Mesh))
            StaticMesh->SetCollisionResponseToChannel(MouseTraceChannel, ECR_Ignore);

        Mesh->SetCastShadow(false);
    }
}

void UOcclusionObstacleComp_Physical::GenerateShadowProxyMeshes()
{
    // Delegate entirely to the shared utility
    UOcclusionMeshUtil::GenerateShadowProxyMeshes(
        GetOwner(),
        NormalMeshes,
        ShadowProxyMaterial,
        StaticShadowProxies,
        SkeletalShadowProxies);

    Modify();
    bool DebugBoolCheck=MarkPackageDirty();
}

void UOcclusionObstacleComp_Physical::DiscoverChildMeshes()
{
    NormalMeshes.Empty();
    OccludedMeshes.Empty();
    NormalStaticMIDs.Empty();
    NormalSkeletalMIDs.Empty();
    OccludedStaticMIDs.Empty();
    OccludedSkeletalMIDs.Empty();
    ActiveOverlaps.Empty();

    UOcclusionMeshUtil::DiscoverChildMeshes(
        this,
        NormalMeshTag,
        OccludedMeshTag,
        NormalMeshes,
        OccludedMeshes);

    UE_LOG(Occlusion, Log,
        TEXT("UOcclusionObstacleComp_Physical::DiscoverChildMeshes>> Normal: %d | Occluded: %d"),
        NormalMeshes.Num(), OccludedMeshes.Num());

    Modify();
}

void UOcclusionObstacleComp_Physical::CleanupInvalidOverlaps()
{
    int32 RemovedCount = 0;
    for (auto It = ActiveOverlaps.CreateIterator(); It; ++It)
        if (!It->IsValid()) { It.RemoveCurrent(); RemovedCount++; }

    if (RemovedCount > 0)
        UE_LOG(Occlusion, Log,
            TEXT("UOcclusionObstacleComp_Physical::CleanupInvalidOverlaps>> Removed %d stale entries"),
            RemovedCount);
}

void UOcclusionObstacleComp_Physical::InitializeMaterials()
{
    UOcclusionMeshUtil::CreateDynamicMaterials_Static  (NormalMeshes,   NormalStaticMIDs);
    UOcclusionMeshUtil::CreateDynamicMaterials_Skeletal(NormalMeshes,   NormalSkeletalMIDs);
    UOcclusionMeshUtil::CreateDynamicMaterials_Static  (OccludedMeshes, OccludedStaticMIDs);
    UOcclusionMeshUtil::CreateDynamicMaterials_Skeletal(OccludedMeshes, OccludedSkeletalMIDs);

    UE_LOG(Occlusion, Log,
        TEXT("UOcclusionObstacleComp_Physical::InitializeMaterials>> NormalStatic:%d NormalSkeletal:%d OccludedStatic:%d OccludedSkeletal:%d"),
        NormalStaticMIDs.Num(), NormalSkeletalMIDs.Num(),
        OccludedStaticMIDs.Num(), OccludedSkeletalMIDs.Num());
}

void UOcclusionObstacleComp_Physical::UpdateMaterialAlpha()
{
    const float NormalAlpha   = 1.f - CurrentAlpha;
    const float OccludedAlpha = CurrentAlpha;

    for (UMaterialInstanceDynamic* MID : NormalStaticMIDs)
        if (MID) MID->SetScalarParameterValue(AlphaParameterName, NormalAlpha);

    for (UMaterialInstanceDynamic* MID : NormalSkeletalMIDs)
        if (MID) MID->SetScalarParameterValue(AlphaParameterName, NormalAlpha);

    for (UMaterialInstanceDynamic* MID : OccludedStaticMIDs)
        if (MID) MID->SetScalarParameterValue(AlphaParameterName, OccludedAlpha);

    for (UMaterialInstanceDynamic* MID : OccludedSkeletalMIDs)
        if (MID) MID->SetScalarParameterValue(AlphaParameterName, OccludedAlpha);
}