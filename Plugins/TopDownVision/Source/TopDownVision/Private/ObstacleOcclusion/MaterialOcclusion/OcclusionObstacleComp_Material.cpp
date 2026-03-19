#include "ObstacleOcclusion/MaterialOcclusion/OcclusionObstacleComp_Material.h"
#include "ObstacleOcclusion/Helper/OcclusionMeshUtil.h"
#include "Components/MeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

DEFINE_LOG_CATEGORY_STATIC(LogMaterialOcclusion, Log, All);

UOcclusionObstacleComp_Material::UOcclusionObstacleComp_Material()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UOcclusionObstacleComp_Material::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogMaterialOcclusion, Log,
        TEXT("UOcclusionObstacleComp_Material::BeginPlay>> %s"),
        *GetOwner()->GetName());

    InitializeMaterials();

    // Start fully visible — both alphas at their visible state
    CurrentAlpha      = 1.f;
    CurrentForceAlpha = 0.f;
    UpdateMaterialAlpha();
}

void UOcclusionObstacleComp_Material::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    const float TargetAlpha      = bShouldBeOccluded ? 0.f : 1.f;
    const float TargetForceAlpha = bForceOccluded    ? 1.f : 0.f;

    CurrentAlpha      = FMath::FInterpTo(CurrentAlpha,      TargetAlpha,      DeltaTime, FadeSpeed);
    CurrentForceAlpha = FMath::FInterpTo(CurrentForceAlpha, TargetForceAlpha, DeltaTime, FadeSpeed);

    UpdateMaterialAlpha();

    const bool bAlphaDone      = FMath::IsNearlyEqual(CurrentAlpha,      TargetAlpha,      0.001f);
    const bool bForceAlphaDone = FMath::IsNearlyEqual(CurrentForceAlpha, TargetForceAlpha, 0.001f);

    if (bAlphaDone && bForceAlphaDone)
    {
        CurrentAlpha      = TargetAlpha;
        CurrentForceAlpha = TargetForceAlpha;
        UpdateMaterialAlpha();
        SetComponentTickEnabled(false);
    }
}

// ── IOcclusionInterface ───────────────────────────────────────────────────────

void UOcclusionObstacleComp_Material::OnOcclusionEnter_Implementation(UObject* SourceTracer)
{
    if (!SourceTracer) return;
    ActiveOverlaps.Add(SourceTracer);
    bShouldBeOccluded = ActiveOverlaps.Num() > 0;
    SetComponentTickEnabled(true);

    UE_LOG(LogMaterialOcclusion, Verbose,
        TEXT("UOcclusionObstacleComp_Material::OnOcclusionEnter>> %s | ActiveOverlaps: %d"),
        *GetOwner()->GetName(), ActiveOverlaps.Num());
}

void UOcclusionObstacleComp_Material::OnOcclusionExit_Implementation(UObject* SourceTracer)
{
    if (!SourceTracer) return;
    ActiveOverlaps.Remove(SourceTracer);
    CleanupInvalidOverlaps();
    
    if (!bForceOccluded)
        bShouldBeOccluded = ActiveOverlaps.Num() > 0;

    SetComponentTickEnabled(true);

    UE_LOG(LogMaterialOcclusion, Verbose,
        TEXT("UOcclusionObstacleComp_Material::OnOcclusionExit>> %s | ActiveOverlaps: %d"),
        *GetOwner()->GetName(), ActiveOverlaps.Num());
}

void UOcclusionObstacleComp_Material::ForceOcclude_Implementation(bool bForce)
{
    bForceOccluded = bForce;
    bShouldBeOccluded = bForce ? true : ActiveOverlaps.Num() > 0;
    SetComponentTickEnabled(true);

    UE_LOG(LogMaterialOcclusion, Log,
        TEXT("UOcclusionObstacleComp_Material::ForceOcclude>> %s | bForce: %s"),
        *GetOwner()->GetName(), bForce ? TEXT("true") : TEXT("false"));
}

// ── Setup ─────────────────────────────────────────────────────────────────────

void UOcclusionObstacleComp_Material::SetupOcclusionMeshes()
{
    DiscoverChildMeshes();

    // only when the shadow cast is required
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

        UE_LOG(LogMaterialOcclusion, Log,
            TEXT("UOcclusionObstacleComp_Material::SetupOcclusionMeshes>> Shadow proxies removed for %s"),
            *GetOwner()->GetName());
    }


    UE_LOG(LogMaterialOcclusion, Log,
        TEXT("UOcclusionObstacleComp_Material::SetupOcclusionMeshes>> Completed for %s"),
        *GetOwner()->GetName());
}

void UOcclusionObstacleComp_Material::InitializeCollisionAndShadow()
{
    for (TSoftObjectPtr<UMeshComponent> MeshPtr : TargetMeshes)
    {
        UMeshComponent* Mesh = MeshPtr.Get();
        if (!Mesh) continue;

        if (UStaticMeshComponent* StaticMesh = Cast<UStaticMeshComponent>(Mesh))
        {
            StaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            StaticMesh->SetCollisionResponseToChannel(OcclusionTraceChannel, ECR_Block);
            StaticMesh->SetCollisionResponseToChannel(MouseTraceChannel, ECR_Ignore);

            UE_LOG(LogMaterialOcclusion, Log,
                TEXT("UOcclusionObstacleComp_Material::InitializeCollisionAndShadow>> ECR_Block set on %s"),
                *StaticMesh->GetName());
        }

        // Source mesh shadow disabled — proxy handles it
        Mesh->SetCastShadow(false);
    }
}

// ── Internal ──────────────────────────────────────────────────────────────────

void UOcclusionObstacleComp_Material::DiscoverChildMeshes()
{
    TargetMeshes.Empty();
    StaticMIDs.Empty();
    SkeletalMIDs.Empty();

    TArray<TSoftObjectPtr<UMeshComponent>> Dummy;

    UOcclusionMeshUtil::DiscoverChildMeshes(
        this,
        MeshTag,
        NAME_None,
        TargetMeshes,
        Dummy);

    UE_LOG(LogMaterialOcclusion, Log,
        TEXT("UOcclusionObstacleComp_Material::DiscoverChildMeshes>> Found %d meshes for %s"),
        TargetMeshes.Num(), *GetOwner()->GetName());
}

void UOcclusionObstacleComp_Material::GenerateShadowProxyMeshes()
{
    UOcclusionMeshUtil::GenerateShadowProxyMeshes(
        GetOwner(),
        TargetMeshes,
        ShadowProxyMaterial,
        StaticShadowProxies,
        SkeletalShadowProxies);

    Modify();
    bool DebugBoolCheck=MarkPackageDirty();
}

void UOcclusionObstacleComp_Material::InitializeMaterials()
{
    UOcclusionMeshUtil::CreateDynamicMaterials_Static  (TargetMeshes, StaticMIDs);
    UOcclusionMeshUtil::CreateDynamicMaterials_Skeletal(TargetMeshes, SkeletalMIDs);

    UE_LOG(LogMaterialOcclusion, Log,
        TEXT("UOcclusionObstacleComp_Material::InitializeMaterials>> Static MIDs: %d | Skeletal MIDs: %d"),
        StaticMIDs.Num(), SkeletalMIDs.Num());
}

void UOcclusionObstacleComp_Material::UpdateMaterialAlpha()
{
    const float ParamAlpha = CurrentAlpha;

    for (UMaterialInstanceDynamic* MID : StaticMIDs)
    {
        if (!MID) continue;
        MID->SetScalarParameterValue(AlphaParameterName, ParamAlpha);
        MID->SetScalarParameterValue(ForceOccludeParameterName, CurrentForceAlpha);
    }

    for (UMaterialInstanceDynamic* MID : SkeletalMIDs)
    {
        if (!MID) continue;
        MID->SetScalarParameterValue(AlphaParameterName, ParamAlpha);
        MID->SetScalarParameterValue(ForceOccludeParameterName, CurrentForceAlpha);
    }
}

void UOcclusionObstacleComp_Material::CleanupInvalidOverlaps()
{
    for (auto It = ActiveOverlaps.CreateIterator(); It; ++It)
        if (!It->IsValid()) It.RemoveCurrent();
}