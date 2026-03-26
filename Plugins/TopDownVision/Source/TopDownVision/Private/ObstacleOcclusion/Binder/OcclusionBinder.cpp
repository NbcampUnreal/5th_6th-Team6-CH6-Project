// Fill out your copyright notice in the Description page of Project Settings.


#include "ObstacleOcclusion/Binder/OcclusionBinder.h"

#include "ObstacleOcclusion/Manager/OcclusionBinderSubsystem.h"
#include "ObstacleOcclusion/Helper/OcclusionMeshUtil.h"
#include "Components/MeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TopDownVisionDebug.h"
#include "Components/SplineMeshComponent.h"
#include "ObstacleOcclusion/MIDPool/OcclusionMIDSlot.h"

DEFINE_LOG_CATEGORY(OcclusionBinder);

AOcclusionBinder::AOcclusionBinder()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────

void AOcclusionBinder::BeginPlay()
{
    Super::BeginPlay();

    // Cache channel settings for runtime use
    OcclusionTraceChannel = UOcclusionMeshUtil::GetOcclusionTraceChannel();
    MouseTraceChannel = UOcclusionMeshUtil::GetMouseTraceChannel();
    
    RegisterToSubsystem();

    // Non-pooled MIDs upfront — bPooled=false, kept for lifetime of actor
    InitializeMaterials();

    CurrentAlpha      = 1.f;
    CurrentForceAlpha = 0.f;
    UpdateMaterialAlpha();
    UpdateOccludedMeshVisibility();// set visibility for occluded mesh

}

void AOcclusionBinder::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Return pooled slots before unregistering so subsystem is still valid
    ReleaseMIDs();

    Super::EndPlay(EndPlayReason);

    if (UOcclusionBinderSubsystem* Sub = GetWorld()->GetSubsystem<UOcclusionBinderSubsystem>())
        Sub->UnregisterBinder(this);
}

void AOcclusionBinder::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

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
        SetActorTickEnabled(false);

        // Fully visible — return pooled slots, non-pooled stay
        if (!bShouldBeOccluded && !bForceOccluded)
            ReleaseMIDs();
    }
}

// ── IOcclusionInterface ───────────────────────────────────────────────────────

void AOcclusionBinder::OnOcclusionEnter_Implementation(UObject* SourceTracer)
{
    if (!SourceTracer) return;

    const bool bWasIdle = ActiveOverlaps.IsEmpty() && !bForceOccluded;
    ActiveOverlaps.Add(SourceTracer);
    bShouldBeOccluded = true;

    if (bWasIdle)
        AcquireMIDs();

    SetActorTickEnabled(true);

    UE_LOG(OcclusionBinder, Log,
        TEXT("AOcclusionBinder::OnOcclusionEnter>> %s | ActiveOverlaps: %d"),
        *GetName(), ActiveOverlaps.Num());
}

void AOcclusionBinder::OnOcclusionExit_Implementation(UObject* SourceTracer)
{
    if (!SourceTracer) return;
    ActiveOverlaps.Remove(SourceTracer);
    CleanupInvalidOverlaps();

    if (!bForceOccluded)
        bShouldBeOccluded = ActiveOverlaps.Num() > 0;

    UpdateOccludedMeshVisibility();

    SetActorTickEnabled(true);

    UE_LOG(OcclusionBinder, Log,
        TEXT("AOcclusionBinder::OnOcclusionExit>> %s | ActiveOverlaps: %d"),
        *GetName(), ActiveOverlaps.Num());
}

void AOcclusionBinder::ForceOcclude_Implementation(bool bForce)
{
    bForceOccluded    = bForce;
    bShouldBeOccluded = bForce || ActiveOverlaps.Num() > 0;

    if (bForce)
        AcquireMIDs();

    UpdateOccludedMeshVisibility();
    
    SetActorTickEnabled(true);

    UE_LOG(OcclusionBinder, Log,
        TEXT("AOcclusionBinder::ForceOcclude>> %s | bForce: %s"),
        *GetName(), bForce ? TEXT("true") : TEXT("false"));
}

// ── Pool ──────────────────────────────────────────────────────────────────────

void AOcclusionBinder::InitializeMaterials()
{
    NormalSlots.Reset();
    OccludedSlots.Reset();
    RTSlots.Reset();

    const FName AlphaParam = UOcclusionMeshUtil::GetAlphaParameterName();
    const FName ForceParam = UOcclusionMeshUtil::GetForceOccludeParameterName();

    UOcclusionMeshUtil::AcquireMaterials(
        NormalMeshes,
        { AlphaParam },
        NormalSlots);
    UOcclusionMeshUtil::AcquireMaterials(
        OccludedMeshes,
        { AlphaParam },
        OccludedSlots);
    UOcclusionMeshUtil::AcquireMaterials(
        RTMeshes,
        { AlphaParam, ForceParam },
        RTSlots);

    UE_LOG(OcclusionBinder, Log,
        TEXT("AOcclusionBinder::InitializeMaterials>> %s | Normal:%d Occluded:%d RT:%d"),
        *GetName(), NormalSlots.Num(), OccludedSlots.Num(), RTSlots.Num());
}

void AOcclusionBinder::AcquireMIDs()
{
    UOcclusionBinderSubsystem* Sub = GetWorld()->GetSubsystem<UOcclusionBinderSubsystem>();
    if (!Sub) return;

    const FName AlphaParam = UOcclusionMeshUtil::GetAlphaParameterName();
    const FName ForceParam = UOcclusionMeshUtil::GetForceOccludeParameterName();

    TArray<FOcclusionMIDSlot> PooledNormal;
    TArray<FOcclusionMIDSlot> PooledOccluded;
    TArray<FOcclusionMIDSlot> PooledRT;

    UOcclusionMeshUtil::AcquireMaterials(
        NormalMeshes,
        { AlphaParam },
        PooledNormal,
        Sub);
    UOcclusionMeshUtil::AcquireMaterials(
        OccludedMeshes,
        { AlphaParam },
        PooledOccluded,
        Sub);
    UOcclusionMeshUtil::AcquireMaterials(
        RTMeshes,
        { AlphaParam, ForceParam },
        PooledRT,
        Sub);

    NormalSlots.Append(PooledNormal);
    OccludedSlots.Append(PooledOccluded);
    RTSlots.Append(PooledRT);

    UpdateMaterialAlpha();

    UE_LOG(OcclusionBinder, Log,
        TEXT("AOcclusionBinder::AcquireMIDs>> %s | Normal:%d Occluded:%d RT:%d"),
        *GetName(), NormalSlots.Num(), OccludedSlots.Num(), RTSlots.Num());
}

void AOcclusionBinder::ReleaseMIDs()
{
    /*UOcclusionBinderSubsystem* Sub = GetWorld()
        ? GetWorld()->GetSubsystem<UOcclusionBinderSubsystem>() : nullptr;

    // Only pooled slots returned — non-pooled stay untouched
    UOcclusionMeshUtil::ReturnMaterials(NormalSlots,   Sub);
    UOcclusionMeshUtil::ReturnMaterials(OccludedSlots, Sub);
    UOcclusionMeshUtil::ReturnMaterials(RTSlots,       Sub);

    UE_LOG(OcclusionBinder, Log,
        TEXT("AOcclusionBinder::ReleaseMIDs>> %s"), *GetName());*/
    UOcclusionBinderSubsystem* Sub = GetWorld()
        ? GetWorld()->GetSubsystem<UOcclusionBinderSubsystem>() : nullptr;

    const FName RTSwitchParam = UOcclusionMeshUtil::GetRTSwitchParameterName();

    // Reset ALL pooled MIDs before returning
    auto ResetSlots = [&](TArray<FOcclusionMIDSlot>& Slots)
    {
        for (FOcclusionMIDSlot& Slot : Slots)
        {
            if (!Slot.IsReady() || !Slot.bPooled) continue;

            // Reset to default (Physical)
            Slot.MID->SetScalarParameterValue(RTSwitchParam, 0.f);
        }
    };

    ResetSlots(NormalSlots);
    ResetSlots(OccludedSlots);
    ResetSlots(RTSlots);

    // Now return them
    UOcclusionMeshUtil::ReturnMaterials(NormalSlots,   Sub);
    UOcclusionMeshUtil::ReturnMaterials(OccludedSlots, Sub);
    UOcclusionMeshUtil::ReturnMaterials(RTSlots,       Sub);

    UE_LOG(OcclusionBinder, Log,
        TEXT("AOcclusionBinder::ReleaseMIDs>> %s"), *GetName());
}

bool AOcclusionBinder::HasPooledMIDs() const
{
    for (const FOcclusionMIDSlot& Slot : NormalSlots)   if (Slot.bPooled) return true;
    for (const FOcclusionMIDSlot& Slot : OccludedSlots) if (Slot.bPooled) return true;
    for (const FOcclusionMIDSlot& Slot : RTSlots)       if (Slot.bPooled) return true;
    return false;
}

// ── Internal ──────────────────────────────────────────────────────────────────

void AOcclusionBinder::UpdateMaterialAlpha()
{
    const FName AlphaParam         = UOcclusionMeshUtil::GetAlphaParameterName();
    const FName ForceParam         = UOcclusionMeshUtil::GetForceOccludeParameterName();
    const FName RTSwitchParam      = UOcclusionMeshUtil::GetRTSwitchParameterName();
    const FName ShouldOccludeParam = UOcclusionMeshUtil::GetOcclusionLockParameterName();

    const float NormalAlpha   = CurrentAlpha;
    const float OccludedAlpha = 1.f - CurrentAlpha;

    for (FOcclusionMIDSlot& Slot : NormalSlots)
    {
        if (!Slot.IsReady()) continue;
        Slot.MID->SetScalarParameterValue(RTSwitchParam, 0.f);
        Slot.MID->SetScalarParameterValue(AlphaParam,    NormalAlpha);
    }

    for (FOcclusionMIDSlot& Slot : OccludedSlots)
    {
        if (!Slot.IsReady()) continue;
        Slot.MID->SetScalarParameterValue(RTSwitchParam, 0.f);
        Slot.MID->SetScalarParameterValue(AlphaParam,    OccludedAlpha);
    }

    for (FOcclusionMIDSlot& Slot : RTSlots)
    {
        if (!Slot.IsReady()) continue;
        Slot.MID->SetScalarParameterValue(RTSwitchParam,      1.f);
        Slot.MID->SetScalarParameterValue(AlphaParam,         NormalAlpha);
        Slot.MID->SetScalarParameterValue(ForceParam,         CurrentForceAlpha);
        Slot.MID->SetScalarParameterValue(ShouldOccludeParam, 1.f); // MID always active — default mat has 0 baked in
    }
}

// ── Editor setup ──────────────────────────────────────────────────────────────

void AOcclusionBinder::SetupBoundActors()
{
      if (BoundActors.IsEmpty())
    {
        UE_LOG(OcclusionBinder, Warning,
            TEXT("AOcclusionBinder::SetupBoundActors>> No bound actors on %s"), *GetName());
        return;
    }

    //get tags and trace channels
    const FName NormalTag   = UOcclusionMeshUtil::GetNormalMeshTag();
    const FName OccludedTag = UOcclusionMeshUtil::GetOccludedMeshTag();
    const FName RTTag       = UOcclusionMeshUtil::GetRTMaterialTag();

    const TEnumAsByte<ECollisionChannel> CachedOcclusionChannel = UOcclusionMeshUtil::GetOcclusionTraceChannel();
    const TEnumAsByte<ECollisionChannel> CachedMouseChannel     = UOcclusionMeshUtil::GetMouseTraceChannel();

    for (AActor* Actor : BoundActors)
    {
        if (!IsValid(Actor)) continue;

        TArray<UMeshComponent*> Meshes;
        Actor->GetComponents<UMeshComponent>(Meshes);

        for (UMeshComponent* Mesh : Meshes)
        {
            if (!Mesh) continue;

            if (UStaticMeshComponent* StaticMesh = Cast<UStaticMeshComponent>(Mesh))
            {
                if (StaticMesh->ComponentHasTag(NormalTag) ||
                    StaticMesh->ComponentHasTag(RTTag))
                {
                    StaticMesh->Modify();
                    StaticMesh->SetCollisionProfileName(TEXT("Custom"));
                    StaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
                    StaticMesh->SetCollisionObjectType(ECC_WorldStatic);
                    StaticMesh->SetCollisionResponseToChannel(CachedOcclusionChannel, ECR_Block);
                    StaticMesh->SetCollisionResponseToChannel(CachedMouseChannel, ECR_Ignore);

                    UE_LOG(OcclusionBinder, Log,
                        TEXT("AOcclusionBinder::SetupBoundActors>> Set occlusion collision on %s from %s"),
                        *StaticMesh->GetName(), *Actor->GetName());
                }
                else if (StaticMesh->ComponentHasTag(OccludedTag))
                {
                    StaticMesh->Modify();
                    StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

                    UE_LOG(OcclusionBinder, Log,
                        TEXT("AOcclusionBinder::SetupBoundActors>> Set no collision on occluded mesh %s from %s"),
                        *StaticMesh->GetName(), *Actor->GetName());
                }

                // spline mesh
                if (USplineMeshComponent* SplineMesh = Cast<USplineMeshComponent>(Mesh))
                {
                    if (SplineMesh->ComponentHasTag(NormalTag) ||
                        SplineMesh->ComponentHasTag(RTTag))
                    {
                        SplineMesh->Modify();
                        SplineMesh->SetCollisionProfileName(TEXT("Custom"));
                        SplineMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
                        SplineMesh->SetCollisionObjectType(ECC_WorldStatic);
                        SplineMesh->SetCollisionResponseToChannel(CachedOcclusionChannel, ECR_Block);
                        SplineMesh->SetCollisionResponseToChannel(CachedMouseChannel, ECR_Ignore);

                        UE_LOG(OcclusionBinder, Log,
                            TEXT("AOcclusionBinder::SetupBoundActors>> Set occlusion collision on SplineMesh %s from %s"),
                            *SplineMesh->GetName(), *Actor->GetName());
                    }
                    continue; // handled — skip the StaticMesh block below
                }
            }
        }

        Actor->Modify();
        bool DebugChecker = Actor->MarkPackageDirty();
    }

    // Editor-time only: discover meshes and generate shadow proxies
    // MIDs are created at runtime BeginPlay — not here
    DiscoverMeshes();
    GenerateShadowProxies();

    UE_LOG(OcclusionBinder, Log,
        TEXT("AOcclusionBinder::SetupBoundActors>> Setup complete for %s | Normal: %d | Occluded: %d | RT: %d"),
        *GetName(), NormalMeshes.Num(), OccludedMeshes.Num(), RTMeshes.Num());
}

void AOcclusionBinder::DiscoverMeshes()
{
    NormalMeshes.Reset();
    OccludedMeshes.Reset();
    RTMeshes.Reset();
    NormalSlots.Reset();
    OccludedSlots.Reset();
    RTSlots.Reset();

    const FName NormalTag   = UOcclusionMeshUtil::GetNormalMeshTag();
    const FName OccludedTag = UOcclusionMeshUtil::GetOccludedMeshTag();
    const FName RTTag       = UOcclusionMeshUtil::GetRTMaterialTag();

    for (AActor* Actor : BoundActors)
    {
        if (!IsValid(Actor)) continue;

        TArray<UMeshComponent*> Meshes;
        Actor->GetComponents<UMeshComponent>(Meshes);

        for (UMeshComponent* Mesh : Meshes)
        {
            if (!Mesh) continue;

            if (Mesh->ComponentHasTag(RTTag))
            {
                // RT only — not added to NormalMeshes
                RTMeshes.Add(Mesh);
            }
            else if (Mesh->ComponentHasTag(NormalTag))
            {
                NormalMeshes.Add(Mesh);
            }
            else if (Mesh->ComponentHasTag(OccludedTag))
            {
                OccludedMeshes.Add(Mesh);
            }
        }
    }

    Modify();

    UE_LOG(OcclusionBinder, Log,
        TEXT("AOcclusionBinder::DiscoverMeshes>> %s | Normal: %d | Occluded: %d | RT: %d"),
        *GetName(), NormalMeshes.Num(), OccludedMeshes.Num(), RTMeshes.Num());
}

void AOcclusionBinder::RegisterToSubsystem()
{
    UOcclusionBinderSubsystem* Sub = GetWorld()->GetSubsystem<UOcclusionBinderSubsystem>();
    if (!Sub) return;

    for (TSoftObjectPtr<UMeshComponent> MeshPtr : NormalMeshes)
    {
        UMeshComponent* Mesh = MeshPtr.Get();
        if (!Mesh) continue;

        if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Mesh))
            Sub->RegisterBinderPrimitive(Prim, this);
    }

    UE_LOG(OcclusionBinder, Log,
        TEXT("AOcclusionBinder::RegisterToSubsystem>> %s | Registered: %d"),
        *GetName(), NormalMeshes.Num());
}

void AOcclusionBinder::GenerateShadowProxies()
{
    for (TObjectPtr<UStaticMeshComponent> Proxy : StaticShadowProxies)
        if (Proxy) Proxy->DestroyComponent();
    StaticShadowProxies.Empty();

    for (TObjectPtr<USkeletalMeshComponent> Proxy : SkeletalShadowProxies)
        if (Proxy) Proxy->DestroyComponent();
    SkeletalShadowProxies.Empty();

    for (AActor* Actor : BoundActors)
    {
        if (!IsValid(Actor)) continue;

        TArray<TSoftObjectPtr<UMeshComponent>> ActorNormalMeshes;
        for (TSoftObjectPtr<UMeshComponent> MeshPtr : NormalMeshes)
        {
            UMeshComponent* Mesh = MeshPtr.Get();
            if (!Mesh) continue;
            if (Mesh->GetOwner() == Actor)
                ActorNormalMeshes.Add(MeshPtr);
        }

        if (ActorNormalMeshes.IsEmpty()) continue;

        TArray<TObjectPtr<UStaticMeshComponent>>  TempStatic;
        TArray<TObjectPtr<USkeletalMeshComponent>> TempSkeletal;

        UOcclusionMeshUtil::GenerateShadowProxyMeshes(
            Actor, ActorNormalMeshes, ShadowProxyMaterial, TempStatic, TempSkeletal);

        StaticShadowProxies.Append(TempStatic);
        SkeletalShadowProxies.Append(TempSkeletal);
    }

    UE_LOG(OcclusionBinder, Log,
        TEXT("AOcclusionBinder::GenerateShadowProxies>> %s | Static: %d | Skeletal: %d"),
        *GetName(), StaticShadowProxies.Num(), SkeletalShadowProxies.Num());
}

void AOcclusionBinder::CleanupInvalidOverlaps()
{
    for (auto It = ActiveOverlaps.CreateIterator(); It; ++It)
        if (!It->IsValid()) It.RemoveCurrent();
}

void AOcclusionBinder::UpdateOccludedMeshVisibility()
{
    /*const float FadeTimeMargine=0.01f;
    const bool bOccluded = CurrentAlpha < 1.f-FadeTimeMargine; */
    
    const bool bOccluded = (bShouldBeOccluded || bForceOccluded);
    
    for (TSoftObjectPtr<UMeshComponent> MeshPtr : OccludedMeshes)
    {
        if (UMeshComponent* Mesh = MeshPtr.Get())
        {
            Mesh->SetHiddenInGame(!bOccluded);
        }
    }
}
