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

    FadeSpeed          = 6.0f;
    AlphaParameterName = TEXT("OcclusionAlpha");
    MeshTag            = TEXT("OcclusionMesh");
}

void UOcclusionObstacleComp_Material::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogMaterialOcclusion, Log,
        TEXT("MaterialOcclusion >> BeginPlay for %s"),
        *GetOwner()->GetName());

    DiscoverChildMeshes();
    InitializeMaterials();
}

void UOcclusionObstacleComp_Material::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    const float TargetAlpha = bShouldBeOccluded ? 1.0f : 0.0f;

    CurrentAlpha = FMath::FInterpTo(CurrentAlpha, TargetAlpha, DeltaTime, FadeSpeed);

    UpdateMaterialAlpha();
}

// ── IOcclusionInterface ───────────────────────────────────────────────────────

void UOcclusionObstacleComp_Material::OnOcclusionEnter_Implementation(UObject* SourceTracer)
{
    if (!SourceTracer) return;

    ActiveOverlaps.Add(SourceTracer);
    bShouldBeOccluded = ActiveOverlaps.Num() > 0;

    UE_LOG(LogMaterialOcclusion, Verbose,
        TEXT("MaterialOcclusion >> Enter %s | overlaps: %d"),
        *GetOwner()->GetName(), ActiveOverlaps.Num());
}

void UOcclusionObstacleComp_Material::OnOcclusionExit_Implementation(UObject* SourceTracer)
{
    if (!SourceTracer) return;

    ActiveOverlaps.Remove(SourceTracer);
    CleanupInvalidOverlaps();
    bShouldBeOccluded = ActiveOverlaps.Num() > 0;

    UE_LOG(LogMaterialOcclusion, Verbose,
        TEXT("MaterialOcclusion >> Exit %s | overlaps: %d"),
        *GetOwner()->GetName(), ActiveOverlaps.Num());
}

// ── Internal ──────────────────────────────────────────────────────────────────

void UOcclusionObstacleComp_Material::DiscoverChildMeshes()
{
    TargetMeshes.Empty();
    DynamicMaterials.Empty();

    UE_LOG(LogMaterialOcclusion, Log,
        TEXT("MaterialOcclusion >> Discovering meshes for %s"),
        *GetOwner()->GetName());

    // OccludedTag unused — material version drives a single alpha on all tagged meshes
    TArray<TSoftObjectPtr<UMeshComponent>> Dummy;

    UOcclusionMeshUtil::DiscoverChildMeshes(
        this,
        MeshTag,
        NAME_None,
        TargetMeshes,
        Dummy);

    UE_LOG(LogMaterialOcclusion, Log,
        TEXT("MaterialOcclusion >> Found %d meshes"),
        TargetMeshes.Num());
}

void UOcclusionObstacleComp_Material::InitializeMaterials()
{
    UOcclusionMeshUtil::CreateDynamicMaterials(TargetMeshes, DynamicMaterials);

    UE_LOG(LogMaterialOcclusion, Log,
        TEXT("MaterialOcclusion >> Created %d dynamic materials"),
        DynamicMaterials.Num());
}

void UOcclusionObstacleComp_Material::UpdateMaterialAlpha()
{
    for (UMaterialInstanceDynamic* MID : DynamicMaterials)
    {
        if (MID) MID->SetScalarParameterValue(AlphaParameterName, CurrentAlpha);
    }
}

void UOcclusionObstacleComp_Material::CleanupInvalidOverlaps()
{
    for (auto It = ActiveOverlaps.CreateIterator(); It; ++It)
    {
        if (!It->IsValid()) It.RemoveCurrent();
    }
}