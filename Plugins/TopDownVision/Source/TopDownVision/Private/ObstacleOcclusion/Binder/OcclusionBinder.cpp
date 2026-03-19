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

    CurrentAlpha = 1.f;
    UpdateMaterialAlpha();
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

    const float TargetAlpha = bShouldBeOccluded ? 0.f : 1.f;
    CurrentAlpha = FMath::FInterpTo(CurrentAlpha, TargetAlpha, DeltaTime, FadeSpeed);

    UpdateMaterialAlpha();

    if (FMath::IsNearlyEqual(CurrentAlpha, TargetAlpha, 0.001f))
    {
        CurrentAlpha = TargetAlpha;
        UpdateMaterialAlpha();
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
    bForceOccluded = bForce;
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

            if (UStaticMeshComponent* StaticMesh = Cast<UStaticMeshComponent>(Mesh))
            {
                if (StaticMesh->ComponentHasTag(NormalMeshTag))
                {
                    StaticMesh->Modify();
                    StaticMesh->SetCollisionProfileName(TEXT("Custom"));
                    StaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
                    StaticMesh->SetCollisionObjectType(ECC_WorldStatic);
                    StaticMesh->SetCollisionResponseToChannel(OcclusionTraceChannel, ECR_Block);
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
        TEXT("AOcclusionBinder::SetupBoundActors>> Setup complete for %s | Bound actors: %d | Normal: %d | Occluded: %d"),
        *GetName(), BoundActors.Num(), NormalMeshes.Num(), OccludedMeshes.Num());
}

// ── Internal ──────────────────────────────────────────────────────────────────

void AOcclusionBinder::DiscoverMeshes()
{
    NormalMeshes.Reset();
    OccludedMeshes.Reset();

    for (AActor* Actor : BoundActors)
    {
        if (!IsValid(Actor)) continue;

        TArray<UMeshComponent*> Meshes;
        Actor->GetComponents<UMeshComponent>(Meshes);

        for (UMeshComponent* Mesh : Meshes)
        {
            if (!Mesh) continue;

            if (Mesh->ComponentHasTag(NormalMeshTag))
                NormalMeshes.Add(Mesh);
            else if (Mesh->ComponentHasTag(OccludedMeshTag))
                OccludedMeshes.Add(Mesh);
        }

        UE_LOG(OcclusionBinder, Log,
            TEXT("AOcclusionBinder::DiscoverMeshes>> %s | Actor: %s | Normal: %d | Occluded: %d"),
            *GetName(), *Actor->GetName(), NormalMeshes.Num(), OccludedMeshes.Num());
    }

    Modify();

    UE_LOG(OcclusionBinder, Log,
        TEXT("AOcclusionBinder::DiscoverMeshes>> %s | Total Normal: %d | Total Occluded: %d"),
        *GetName(), NormalMeshes.Num(), OccludedMeshes.Num());
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
        TEXT("AOcclusionBinder::RegisterToSubsystem>> %s | Registered %d primitives"),
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
    NormalDynamicMaterials.Reset();
    OccludedDynamicMaterials.Reset();
    NormalIsRTMaterial.Reset();
    OccludedIsRTMaterial.Reset();

    auto CreateMIDs = [this](
        const TArray<TSoftObjectPtr<UMeshComponent>>& Meshes,
        TArray<UMaterialInstanceDynamic*>& OutMIDs,
        TArray<bool>& OutIsRT)
    {
        for (TSoftObjectPtr<UMeshComponent> MeshPtr : Meshes)
        {
            UMeshComponent* Mesh = MeshPtr.Get();
            if (!Mesh) continue;

            const bool bIsRT = Mesh->ComponentHasTag(RTMaterialTag);

            for (int32 i = 0; i < Mesh->GetNumMaterials(); ++i)
            {
                UMaterialInterface* Mat = Mesh->GetMaterial(i);
                if (!Mat) continue;

                UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Mat, this);
                if (!MID) continue;

                Mesh->SetMaterial(i, MID);
                OutMIDs.Add(MID);
                OutIsRT.Add(bIsRT);
            }
        }
    };

    CreateMIDs(NormalMeshes,   NormalDynamicMaterials,   NormalIsRTMaterial);
    CreateMIDs(OccludedMeshes, OccludedDynamicMaterials, OccludedIsRTMaterial);

    UE_LOG(OcclusionBinder, Log,
        TEXT("AOcclusionBinder::InitializeMaterials>> %s | Normal MIDs: %d | Occluded MIDs: %d"),
        *GetName(), NormalDynamicMaterials.Num(), OccludedDynamicMaterials.Num());
}

void AOcclusionBinder::UpdateMaterialAlpha()
{
    // CurrentAlpha: 1 = visible, 0 = occluded
    // Material expects: 1 = visible, 0 = occluded
    // So pass CurrentAlpha directly to normal meshes — no inversion needed
    const float NormalParamAlpha   = CurrentAlpha;
    // Occluded visual meshes fade IN — opposite direction
    const float OccludedParamAlpha = 1.f - CurrentAlpha;
    const float ForceParam         = bForceOccluded ? 1.f : 0.f;

    for (int32 i = 0; i < NormalDynamicMaterials.Num(); ++i)
    {
        UMaterialInstanceDynamic* MID = NormalDynamicMaterials[i];
        if (!MID) continue;

        MID->SetScalarParameterValue(AlphaParameterName, NormalParamAlpha);

        if (NormalIsRTMaterial.IsValidIndex(i) && NormalIsRTMaterial[i])
            MID->SetScalarParameterValue(ForceOccludeParameterName, ForceParam);
    }

    for (int32 i = 0; i < OccludedDynamicMaterials.Num(); ++i)
    {
        UMaterialInstanceDynamic* MID = OccludedDynamicMaterials[i];
        if (!MID) continue;

        MID->SetScalarParameterValue(AlphaParameterName, OccludedParamAlpha);

        if (OccludedIsRTMaterial.IsValidIndex(i) && OccludedIsRTMaterial[i])
            MID->SetScalarParameterValue(ForceOccludeParameterName, ForceParam);
    }
}

void AOcclusionBinder::CleanupInvalidOverlaps()
{
    for (auto It = ActiveOverlaps.CreateIterator(); It; ++It)
        if (!It->IsValid()) It.RemoveCurrent();
}