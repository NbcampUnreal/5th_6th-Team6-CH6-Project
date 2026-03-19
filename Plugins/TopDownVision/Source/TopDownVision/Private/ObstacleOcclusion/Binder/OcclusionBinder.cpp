#include "ObstacleOcclusion/Binder/OcclusionBinder.h"
#include "ObstacleOcclusion/Manager/OcclusionBinderSubsystem.h"
#include "ObstacleOcclusion/Helper/OcclusionMeshUtil.h"
#include "Components/MeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TopDownVisionDebug.h"

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

    RegisterToSubsystem();
    InitializeMaterials();

    CurrentAlpha      = 1.f;
    CurrentForceAlpha = 0.f;
    UpdateMaterialAlpha();
    //UpdateMouseTraceCollision(false);
}

void AOcclusionBinder::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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
       // UpdateMouseTraceCollision(bShouldBeOccluded);
        SetActorTickEnabled(false);
    }
}

// ── IOcclusionInterface ───────────────────────────────────────────────────────

void AOcclusionBinder::OnOcclusionEnter_Implementation(UObject* SourceTracer)
{
    if (!SourceTracer) return;
    ActiveOverlaps.Add(SourceTracer);
    bShouldBeOccluded = ActiveOverlaps.Num() > 0;
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

    SetActorTickEnabled(true);

    UE_LOG(OcclusionBinder, Log,
        TEXT("AOcclusionBinder::OnOcclusionExit>> %s | ActiveOverlaps: %d"),
        *GetName(), ActiveOverlaps.Num());
}

void AOcclusionBinder::ForceOcclude_Implementation(bool bForce)
{
    bForceOccluded    = bForce;
    bShouldBeOccluded = bForce ? true : ActiveOverlaps.Num() > 0;
    SetActorTickEnabled(true);

    UE_LOG(OcclusionBinder, Log,
        TEXT("AOcclusionBinder::ForceOcclude>> %s | bForce: %s"),
        *GetName(), bForce ? TEXT("true") : TEXT("false"));
}

// ── Editor setup ──────────────────────────────────────────────────────────────

void AOcclusionBinder::SetupBoundActors()
{
    if (BoundActors.IsEmpty())
    {
        UE_LOG(OcclusionBinder, Warning,
            TEXT("AOcclusionBinder::SetupBoundActors>> No bound actors on %s"),
            *GetName());
        return;
    }

    for (AActor* Actor : BoundActors)
    {
        if (!IsValid(Actor)) continue;

        TArray<UMeshComponent*> Meshes;
        Actor->GetComponents<UMeshComponent>(Meshes);

        for (UMeshComponent* Mesh : Meshes)
        {
            if (!Mesh) continue;

            // Cast catches both UStaticMeshComponent and USplineMeshComponent
            if (UStaticMeshComponent* StaticMesh = Cast<UStaticMeshComponent>(Mesh))
            {
                if (StaticMesh->ComponentHasTag(NormalMeshTag) ||
                    StaticMesh->ComponentHasTag(RTMaterialTag))
                {
                    StaticMesh->Modify();
                    StaticMesh->SetCollisionProfileName(TEXT("Custom"));
                    StaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
                    StaticMesh->SetCollisionObjectType(ECC_WorldStatic);
                    StaticMesh->SetCollisionResponseToChannel(OcclusionTraceChannel, ECR_Block);
                    // Mouse trace starts as Ignore — switches to Block when occluded
                    //TODO :: make safe way to ignore the mouse trace channel
                    StaticMesh->SetCollisionResponseToChannel(MouseTraceChannel, ECR_Ignore);

                    UE_LOG(OcclusionBinder, Log,
                        TEXT("AOcclusionBinder::SetupBoundActors>> Set occlusion collision on %s from %s"),
                        *StaticMesh->GetName(), *Actor->GetName());
                }
                else if (StaticMesh->ComponentHasTag(OccludedMeshTag))
                {
                    StaticMesh->Modify();
                    StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

                    UE_LOG(OcclusionBinder, Log,
                        TEXT("AOcclusionBinder::SetupBoundActors>> Set no collision on occluded mesh %s from %s"),
                        *StaticMesh->GetName(), *Actor->GetName());
                }
            }
        }

        Actor->Modify();
        Actor->MarkPackageDirty();
    }

    DiscoverMeshes();
    GenerateShadowProxies();

    UE_LOG(OcclusionBinder, Log,
        TEXT("AOcclusionBinder::SetupBoundActors>> Setup complete for %s | Normal: %d | Occluded: %d | RT: %d"),
        *GetName(), NormalMeshes.Num(), OccludedMeshes.Num(), RTMeshes.Num());
}

// ── Internal ──────────────────────────────────────────────────────────────────

void AOcclusionBinder::DiscoverMeshes()
{
    NormalMeshes.Reset();
    OccludedMeshes.Reset();
    RTMeshes.Reset();

    for (AActor* Actor : BoundActors)
    {
        if (!IsValid(Actor)) continue;

        TArray<UMeshComponent*> Meshes;
        Actor->GetComponents<UMeshComponent>(Meshes);

        for (UMeshComponent* Mesh : Meshes)
        {
            if (!Mesh) continue;

            if (Mesh->ComponentHasTag(RTMaterialTag))
            {
                NormalMeshes.Add(Mesh);
                RTMeshes.Add(Mesh);
            }
            else if (Mesh->ComponentHasTag(NormalMeshTag))
            {
                NormalMeshes.Add(Mesh);
            }
            else if (Mesh->ComponentHasTag(OccludedMeshTag))
            {
                OccludedMeshes.Add(Mesh);
            }
        }

        UE_LOG(OcclusionBinder, Log,
            TEXT("AOcclusionBinder::DiscoverMeshes>> %s | Actor: %s | Normal: %d | Occluded: %d | RT: %d"),
            *GetName(), *Actor->GetName(), NormalMeshes.Num(), OccludedMeshes.Num(), RTMeshes.Num());
    }

    Modify();

    UE_LOG(OcclusionBinder, Log,
        TEXT("AOcclusionBinder::DiscoverMeshes>> %s | Total Normal: %d | Occluded: %d | RT: %d"),
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

void AOcclusionBinder::InitializeMaterials()
{
    NormalStaticMIDs.Reset();
    NormalSkeletalMIDs.Reset();
    OccludedStaticMIDs.Reset();
    OccludedSkeletalMIDs.Reset();
    RTStaticMIDs.Reset();
    RTSkeletalMIDs.Reset();

    TArray<TSoftObjectPtr<UMeshComponent>> PureNormalMeshes;
    for (TSoftObjectPtr<UMeshComponent> MeshPtr : NormalMeshes)
    {
        UMeshComponent* Mesh = MeshPtr.Get();
        if (!Mesh) continue;
        if (!Mesh->ComponentHasTag(RTMaterialTag))
            PureNormalMeshes.Add(MeshPtr);
    }

    UOcclusionMeshUtil::CreateDynamicMaterials_Static  (PureNormalMeshes, NormalStaticMIDs);
    UOcclusionMeshUtil::CreateDynamicMaterials_Skeletal(PureNormalMeshes, NormalSkeletalMIDs);

    UOcclusionMeshUtil::CreateDynamicMaterials_Static  (OccludedMeshes, OccludedStaticMIDs);
    UOcclusionMeshUtil::CreateDynamicMaterials_Skeletal(OccludedMeshes, OccludedSkeletalMIDs);

    UOcclusionMeshUtil::CreateDynamicMaterials_Static  (RTMeshes, RTStaticMIDs);
    UOcclusionMeshUtil::CreateDynamicMaterials_Skeletal(RTMeshes, RTSkeletalMIDs);

    UE_LOG(OcclusionBinder, Log,
        TEXT("AOcclusionBinder::InitializeMaterials>> %s | NormalStatic:%d NormalSkeletal:%d OccludedStatic:%d OccludedSkeletal:%d RTStatic:%d RTSkeletal:%d"),
        *GetName(),
        NormalStaticMIDs.Num(), NormalSkeletalMIDs.Num(),
        OccludedStaticMIDs.Num(), OccludedSkeletalMIDs.Num(),
        RTStaticMIDs.Num(), RTSkeletalMIDs.Num());
}

void AOcclusionBinder::UpdateMaterialAlpha()
{
    const float NormalAlpha   = CurrentAlpha;
    const float OccludedAlpha = 1.f - CurrentAlpha;

    for (UMaterialInstanceDynamic* MID : NormalStaticMIDs)
        if (MID) MID->SetScalarParameterValue(AlphaParameterName, NormalAlpha);

    for (UMaterialInstanceDynamic* MID : NormalSkeletalMIDs)
        if (MID) MID->SetScalarParameterValue(AlphaParameterName, NormalAlpha);

    for (UMaterialInstanceDynamic* MID : OccludedStaticMIDs)
        if (MID) MID->SetScalarParameterValue(AlphaParameterName, OccludedAlpha);

    for (UMaterialInstanceDynamic* MID : OccludedSkeletalMIDs)
        if (MID) MID->SetScalarParameterValue(AlphaParameterName, OccludedAlpha);

    for (UMaterialInstanceDynamic* MID : RTStaticMIDs)
    {
        if (!MID) continue;
        MID->SetScalarParameterValue(AlphaParameterName,        NormalAlpha);
        MID->SetScalarParameterValue(ForceOccludeParameterName, CurrentForceAlpha);
    }

    for (UMaterialInstanceDynamic* MID : RTSkeletalMIDs)
    {
        if (!MID) continue;
        MID->SetScalarParameterValue(AlphaParameterName,        NormalAlpha);
        MID->SetScalarParameterValue(ForceOccludeParameterName, CurrentForceAlpha);
    }
}

/*void AOcclusionBinder::UpdateMouseTraceCollision(bool bOccluded)
{
    const ECollisionResponse Response = bOccluded ? ECR_Block : ECR_Ignore;

    for (TSoftObjectPtr<UMeshComponent> MeshPtr : NormalMeshes)
    {
        UMeshComponent* Mesh = MeshPtr.Get();
        if (!Mesh) continue;

        if (UStaticMeshComponent* StaticMesh = Cast<UStaticMeshComponent>(Mesh))
            StaticMesh->SetCollisionResponseToChannel(MouseTraceChannel, Response);
    }

    UE_LOG(OcclusionBinder, Log,
        TEXT("AOcclusionBinder::UpdateMouseTraceCollision>> %s | %s"),
        *GetName(), bOccluded ? TEXT("Block") : TEXT("Ignore"));
}*/

void AOcclusionBinder::CleanupInvalidOverlaps()
{
    for (auto It = ActiveOverlaps.CreateIterator(); It; ++It)
        if (!It->IsValid()) It.RemoveCurrent();
}