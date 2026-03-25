// Fill out your copyright notice in the Description page of Project Settings.

#include "TopDownVision/Public/ObstacleOcclusion/PhysicalOcclusion/OcclusionObstacleComp_Physical.h"

#include "Components/MeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ObstacleOcclusion/Helper/OcclusionMeshUtil.h"
#include "ObstacleOcclusion/Manager/OcclusionBinderSubsystem.h"
#include "TopDownVisionDebug.h"

UOcclusionObstacleComp_Physical::UOcclusionObstacleComp_Physical()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────

void UOcclusionObstacleComp_Physical::BeginPlay()
{
    Super::BeginPlay();

    OcclusionTraceChannel = UOcclusionMeshUtil::GetOcclusionTraceChannel();
    MouseTraceChannel     = UOcclusionMeshUtil::GetMouseTraceChannel();
    
    // Create non-pooled MIDs upfront — bPooled=false, kept for lifetime of comp
    InitializeMaterials();

    CurrentAlpha = 1.f;
    UpdateMaterialAlpha();

}

void UOcclusionObstacleComp_Physical::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Return pooled slots before super so subsystem is still valid
    // Non-pooled slots are skipped by ReturnMaterials — GC handles them
    ReleaseMIDs();
    Super::EndPlay(EndPlayReason);
}

void UOcclusionObstacleComp_Physical::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    const float TargetAlpha = bShouldBeOccluded ? 0.f : 1.f;

    if (bShouldBeOccluded != bLastOcclusionState)
    {
        UE_LOG(Occlusion, Verbose,
            TEXT("UOcclusionObstacleComp_Physical::Tick>> State -> %s"),
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

        // Fully visible — return pooled slots, non-pooled stay
        if (!bShouldBeOccluded && !bForceOccluded)
            ReleaseMIDs();
    }
}

// ── IOcclusionInterface ───────────────────────────────────────────────────────

void UOcclusionObstacleComp_Physical::OnOcclusionEnter_Implementation(UObject* SourceTracer)
{
    if (!SourceTracer) return;

    const bool bWasIdle = ActiveOverlaps.IsEmpty() && !bForceOccluded;
    ActiveOverlaps.Add(SourceTracer);
    bShouldBeOccluded = true;

    // Checkout pooled MIDs on first entering tracer
    if (bWasIdle)
        AcquireMIDs();

    SetComponentTickEnabled(true);

    UE_LOG(Occlusion, Log,
        TEXT("UOcclusionObstacleComp_Physical::OnOcclusionEnter>> %s | ActiveOverlaps: %d"),
        *GetOwner()->GetName(), ActiveOverlaps.Num());
}

void UOcclusionObstacleComp_Physical::OnOcclusionExit_Implementation(UObject* SourceTracer)
{
    if (!SourceTracer) return;
    ActiveOverlaps.Remove(SourceTracer);
    CleanupInvalidOverlaps();

    if (!bForceOccluded)
        bShouldBeOccluded = ActiveOverlaps.Num() > 0;

    SetComponentTickEnabled(true);

    UE_LOG(Occlusion, Log,
        TEXT("UOcclusionObstacleComp_Physical::OnOcclusionExit>> %s | ActiveOverlaps: %d"),
        *GetOwner()->GetName(), ActiveOverlaps.Num());
}

void UOcclusionObstacleComp_Physical::ForceOcclude_Implementation(bool bForce)
{
    bForceOccluded    = bForce;
    bShouldBeOccluded = bForce || ActiveOverlaps.Num() > 0;

    if (bForce)
        AcquireMIDs();

    SetComponentTickEnabled(true);

    UE_LOG(Occlusion, Log,
        TEXT("UOcclusionObstacleComp_Physical::ForceOcclude>> %s | bForce: %s"),
        *GetOwner()->GetName(), bForce ? TEXT("true") : TEXT("false"));
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
    const TEnumAsByte<ECollisionChannel> OcclusionCh = UOcclusionMeshUtil::GetOcclusionTraceChannel();
    const TEnumAsByte<ECollisionChannel> MouseCh     = UOcclusionMeshUtil::GetMouseTraceChannel();
    
    for (TSoftObjectPtr<UMeshComponent> MeshPtr : NormalMeshes)
    {
        UMeshComponent* Mesh = MeshPtr.Get();
        if (!Mesh) continue;

        if (UStaticMeshComponent* StaticMesh = Cast<UStaticMeshComponent>(Mesh))
        {
            StaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            StaticMesh->SetCollisionResponseToChannel(OcclusionCh, ECR_Block);
            StaticMesh->SetCollisionResponseToChannel(MouseCh, ECR_Ignore);
        }

        Mesh->SetCastShadow(false);
    }

    for (TSoftObjectPtr<UMeshComponent> MeshPtr : OccludedMeshes)
    {
        UMeshComponent* Mesh = MeshPtr.Get();
        if (!Mesh) continue;

        if (UStaticMeshComponent* StaticMesh = Cast<UStaticMeshComponent>(Mesh))
            StaticMesh->SetCollisionResponseToChannel(MouseCh, ECR_Ignore);

        Mesh->SetCastShadow(false);
    }
}

// ── Pool ──────────────────────────────────────────────────────────────────────

void UOcclusionObstacleComp_Physical::InitializeMaterials()
{
    // No pool passed — creates directly, bPooled=false on all slots
    // These slots stay for the lifetime of the comp
    const FName AlphaParam = UOcclusionMeshUtil::GetAlphaParameterName();

    UOcclusionMeshUtil::AcquireMaterials(NormalMeshes,   { AlphaParam }, NormalSlots);
    UOcclusionMeshUtil::AcquireMaterials(OccludedMeshes, { AlphaParam }, OccludedSlots);

    UE_LOG(Occlusion, Log,
        TEXT("UOcclusionObstacleComp_Physical::InitializeMaterials>> Normal:%d Occluded:%d"),
        NormalSlots.Num(), OccludedSlots.Num());
}

void UOcclusionObstacleComp_Physical::AcquireMIDs()
{
    UOcclusionBinderSubsystem* Sub = GetWorld()->GetSubsystem<UOcclusionBinderSubsystem>();
    if (!Sub) return;

    const FName AlphaParam = UOcclusionMeshUtil::GetAlphaParameterName();

    TArray<FOcclusionMIDSlot> PooledNormal;
    TArray<FOcclusionMIDSlot> PooledOccluded;

    UOcclusionMeshUtil::AcquireMaterials(NormalMeshes,   { AlphaParam }, PooledNormal,   Sub);
    UOcclusionMeshUtil::AcquireMaterials(OccludedMeshes, { AlphaParam }, PooledOccluded, Sub);

    NormalSlots.Append(PooledNormal);
    OccludedSlots.Append(PooledOccluded);

    UpdateMaterialAlpha();

    UE_LOG(Occlusion, Log,
        TEXT("UOcclusionObstacleComp_Physical::AcquireMIDs>> %s | Normal:%d Occluded:%d"),
        *GetOwner()->GetName(), NormalSlots.Num(), OccludedSlots.Num());
}

void UOcclusionObstacleComp_Physical::ReleaseMIDs()
{
    UOcclusionBinderSubsystem* Sub = GetWorld()
        ? GetWorld()->GetSubsystem<UOcclusionBinderSubsystem>() : nullptr;

    // ReturnMaterials only acts on bPooled=true slots — non-pooled are skipped
    UOcclusionMeshUtil::ReturnMaterials(NormalSlots,   Sub);
    UOcclusionMeshUtil::ReturnMaterials(OccludedSlots, Sub);

    UE_LOG(Occlusion, Log,
        TEXT("UOcclusionObstacleComp_Physical::ReleaseMIDs>> %s"), *GetOwner()->GetName());
}

bool UOcclusionObstacleComp_Physical::HasMIDs() const
{
    // Check for any pooled slot specifically — non-pooled are always present
    for (const FOcclusionMIDSlot& Slot : NormalSlots)
        if (Slot.bPooled) return true;
    for (const FOcclusionMIDSlot& Slot : OccludedSlots)
        if (Slot.bPooled) return true;
    return false;
}

// ── Internal ──────────────────────────────────────────────────────────────────

void UOcclusionObstacleComp_Physical::GenerateShadowProxyMeshes()
{
    UOcclusionMeshUtil::GenerateShadowProxyMeshes(
        GetOwner(),
        NormalMeshes,
        ShadowProxyMaterial,
        StaticShadowProxies,
        SkeletalShadowProxies);

    Modify();
    MarkPackageDirty();
}

void UOcclusionObstacleComp_Physical::DiscoverChildMeshes()
{
    NormalMeshes.Empty();
    OccludedMeshes.Empty();
    NormalSlots.Empty();
    OccludedSlots.Empty();
    ActiveOverlaps.Empty();

    UOcclusionMeshUtil::DiscoverChildMeshes(
        this,
        NormalMeshes,
        OccludedMeshes);

    UE_LOG(Occlusion, Log,
        TEXT("UOcclusionObstacleComp_Physical::DiscoverChildMeshes>> Normal: %d | Occluded: %d"),
        NormalMeshes.Num(), OccludedMeshes.Num());

    Modify();
}

void UOcclusionObstacleComp_Physical::UpdateMaterialAlpha()
{
    const FName AlphaParam = UOcclusionMeshUtil::GetAlphaParameterName();

    const float NormalAlpha   =  CurrentAlpha;
    const float OccludedAlpha = 1.f - CurrentAlpha;

    for (FOcclusionMIDSlot& Slot : NormalSlots)
        if (Slot.IsReady())
            Slot.MID->SetScalarParameterValue(AlphaParam, NormalAlpha);

    for (FOcclusionMIDSlot& Slot : OccludedSlots)
        if (Slot.IsReady())
            Slot.MID->SetScalarParameterValue(AlphaParam, OccludedAlpha);
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