#include "ObstacleOcclusion/MaterialOcclusion/OcclusionObstacleComp_Material.h"
#include "ObstacleOcclusion/Helper/OcclusionMeshUtil.h"
#include "ObstacleOcclusion/Manager/OcclusionBinderSubsystem.h"
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

// ── Lifecycle ─────────────────────────────────────────────────────────────────

void UOcclusionObstacleComp_Material::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogMaterialOcclusion, Verbose,
        TEXT("UOcclusionObstacleComp_Material::BeginPlay>> %s"),
        *GetOwner()->GetName());

    OcclusionTraceChannel = UOcclusionMeshUtil::GetOcclusionTraceChannel();
    MouseTraceChannel     = UOcclusionMeshUtil::GetMouseTraceChannel();

    // Non-pooled MIDs — bPooled=false, kept for lifetime of comp
    InitializeMaterials();

    // force update alpha to mid
    CurrentAlpha      = 1.f;
    CurrentForceAlpha = 0.f;
    UpdateMaterialAlpha();
}

void UOcclusionObstacleComp_Material::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Return pooled slots before super so subsystem is still valid
    // Non-pooled slots skipped by ReturnMaterials automatically
    ReleaseMIDs();
    Super::EndPlay(EndPlayReason);
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

        // Fully visible — return pooled slots, non-pooled stay
        if (!bShouldBeOccluded && !bForceOccluded)
            ReleaseMIDs();
    }
}

// ── IOcclusionInterface ───────────────────────────────────────────────────────

void UOcclusionObstacleComp_Material::OnOcclusionEnter_Implementation(UObject* SourceTracer)
{
    if (!SourceTracer) return;

    const bool bWasIdle = ActiveOverlaps.IsEmpty() && !bForceOccluded;
    ActiveOverlaps.Add(SourceTracer);
    bShouldBeOccluded = true;

    // Checkout pooled MIDs on first entering tracer
    if (bWasIdle)
        AcquireMIDs();

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
    bForceOccluded    = bForce;
    bShouldBeOccluded = bForce || ActiveOverlaps.Num() > 0;

    if (bForce)
        AcquireMIDs();

    SetComponentTickEnabled(true);

    UE_LOG(LogMaterialOcclusion, Verbose,
        TEXT("UOcclusionObstacleComp_Material::ForceOcclude>> %s | bForce: %s"),
        *GetOwner()->GetName(), bForce ? TEXT("true") : TEXT("false"));
}

// ── Setup ─────────────────────────────────────────────────────────────────────

void UOcclusionObstacleComp_Material::SetupOcclusionMeshes()
{
    DiscoverChildMeshes();

    if (bCastShadowWhenOccluded)
    {
        GenerateShadowProxyMeshes();
    }
    else
    {
        for (TObjectPtr<UStaticMeshComponent> Proxy : StaticShadowProxies)
            if (Proxy) Proxy->DestroyComponent();
        StaticShadowProxies.Empty();

        for (TObjectPtr<USkeletalMeshComponent> Proxy : SkeletalShadowProxies)
            if (Proxy) Proxy->DestroyComponent();
        SkeletalShadowProxies.Empty();

        UE_LOG(LogMaterialOcclusion, Verbose,
            TEXT("UOcclusionObstacleComp_Material::SetupOcclusionMeshes>> Shadow proxies removed for %s"),
            *GetOwner()->GetName());
    }

    UE_LOG(LogMaterialOcclusion, Verbose,
        TEXT("UOcclusionObstacleComp_Material::SetupOcclusionMeshes>> Completed for %s"),
        *GetOwner()->GetName());
}

void UOcclusionObstacleComp_Material::InitializeCollisionAndShadow()
{
    const TEnumAsByte<ECollisionChannel> OcclusionCh = UOcclusionMeshUtil::GetOcclusionTraceChannel();
    const TEnumAsByte<ECollisionChannel> MouseCh     = UOcclusionMeshUtil::GetMouseTraceChannel();
    
    for (TSoftObjectPtr<UMeshComponent> MeshPtr : TargetMeshes)
    {
        UMeshComponent* Mesh = MeshPtr.Get();
        if (!Mesh) continue;

        if (UStaticMeshComponent* StaticMesh = Cast<UStaticMeshComponent>(Mesh))
        {
            StaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            StaticMesh->SetCollisionResponseToChannel(OcclusionCh, ECR_Block);
            StaticMesh->SetCollisionResponseToChannel(MouseCh, ECR_Ignore);

            UE_LOG(LogMaterialOcclusion, Verbose,
                TEXT("UOcclusionObstacleComp_Material::InitializeCollisionAndShadow>> ECR_Block set on %s"),
                *StaticMesh->GetName());
        }

        Mesh->SetCastShadow(false);
    }
}

// ── Pool ──────────────────────────────────────────────────────────────────────

void UOcclusionObstacleComp_Material::InitializeMaterials()
{
    // Both params required — material comp drives AlphaParameterName AND ForceOccludeParameterName
    // No pool passed — bPooled=false, kept for lifetime of comp
    const FName AlphaParam = UOcclusionMeshUtil::GetAlphaParameterName();
    const FName ForceParam = UOcclusionMeshUtil::GetForceOccludeParameterName();

    UOcclusionMeshUtil::AcquireMaterials(
        TargetMeshes,
        { AlphaParam, ForceParam },
        TargetSlots);

    UE_LOG(LogMaterialOcclusion, Verbose,
        TEXT("UOcclusionObstacleComp_Material::InitializeMaterials>> Slots: %d"),
        TargetSlots.Num());
}

void UOcclusionObstacleComp_Material::AcquireMIDs()
{
    UOcclusionBinderSubsystem* Sub = GetWorld()->GetSubsystem<UOcclusionBinderSubsystem>();
    if (!Sub) return;

    const FName AlphaParam = UOcclusionMeshUtil::GetAlphaParameterName();
    const FName ForceParam = UOcclusionMeshUtil::GetForceOccludeParameterName();

    TArray<FOcclusionMIDSlot> PooledSlots;
    UOcclusionMeshUtil::AcquireMaterials(
        TargetMeshes,
        { AlphaParam, ForceParam },
        PooledSlots,
        Sub);

    TargetSlots.Append(PooledSlots);

    UpdateMaterialAlpha();

    UE_LOG(LogMaterialOcclusion, Verbose,
        TEXT("UOcclusionObstacleComp_Material::AcquireMIDs>> %s | Total slots: %d"),
        *GetOwner()->GetName(), TargetSlots.Num());
}

void UOcclusionObstacleComp_Material::ReleaseMIDs()
{
    UOcclusionBinderSubsystem* Sub = GetWorld()
        ? GetWorld()->GetSubsystem<UOcclusionBinderSubsystem>() : nullptr;

    // Only pooled slots are returned — non-pooled stay untouched
    UOcclusionMeshUtil::ReturnMaterials(TargetSlots, Sub);

    UE_LOG(LogMaterialOcclusion, Verbose,
        TEXT("UOcclusionObstacleComp_Material::ReleaseMIDs>> %s"), *GetOwner()->GetName());
}

bool UOcclusionObstacleComp_Material::HasPooledMIDs() const
{
    for (const FOcclusionMIDSlot& Slot : TargetSlots)
        if (Slot.bPooled) return true;
    return false;
}

// ── Internal ──────────────────────────────────────────────────────────────────

void UOcclusionObstacleComp_Material::DiscoverChildMeshes()
{
    TargetMeshes.Empty();
    TargetSlots.Empty();

    TArray<TSoftObjectPtr<UMeshComponent>> Dummy;
    UOcclusionMeshUtil::DiscoverChildMeshes(
        this,
        TargetMeshes,
        Dummy);

    UE_LOG(LogMaterialOcclusion, Verbose,
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
    bool DebugBoolChecker=MarkPackageDirty();
}

void UOcclusionObstacleComp_Material::UpdateMaterialAlpha()
{
    const FName AlphaParam = UOcclusionMeshUtil::GetAlphaParameterName();
    const FName ForceParam = UOcclusionMeshUtil::GetForceOccludeParameterName();

    for (FOcclusionMIDSlot& Slot : TargetSlots)
    {
        if (!Slot.IsReady()) continue;
        Slot.MID->SetScalarParameterValue(AlphaParam, CurrentAlpha);
        Slot.MID->SetScalarParameterValue(ForceParam, CurrentForceAlpha);
    }
}

void UOcclusionObstacleComp_Material::CleanupInvalidOverlaps()
{
    for (auto It = ActiveOverlaps.CreateIterator(); It; ++It)
        if (!It->IsValid()) It.RemoveCurrent();
}