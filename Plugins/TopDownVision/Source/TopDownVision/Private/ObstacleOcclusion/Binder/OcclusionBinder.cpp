#include "ObstacleOcclusion/Binder/OcclusionBinder.h"
#include "ObstacleOcclusion/Manager/OcclusionBinderSubsystem.h"
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

    DiscoverAndRegisterMeshes();
    InitializeMaterials();
}

void AOcclusionBinder::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    // Unregister all primitives from subsystem on cleanup
    if (UOcclusionBinderSubsystem* Sub = GetWorld()->GetSubsystem<UOcclusionBinderSubsystem>())
    {
        Sub->UnregisterBinder(this);
    }
}

void AOcclusionBinder::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    const float TargetAlpha = bShouldBeOccluded ? 1.f : 0.f;

    CurrentAlpha = FMath::FInterpTo(CurrentAlpha, TargetAlpha, DeltaTime, FadeSpeed);

    UpdateMaterialAlpha();

    // Stop ticking once lerp has settled
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
    bShouldBeOccluded = ActiveOverlaps.Num() > 0;

    SetActorTickEnabled(true);

    UE_LOG(OcclusionBinder, Log,
        TEXT("AOcclusionBinder::OnOcclusionExit>> %s | ActiveOverlaps: %d"),
        *GetName(), ActiveOverlaps.Num());
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

            // Collision — static only, skeletal uses physics asset
            if (UStaticMeshComponent* StaticMesh = Cast<UStaticMeshComponent>(Mesh))
            {
                StaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
                StaticMesh->SetCollisionResponseToChannel(OcclusionTraceChannel, ECR_Block);
                StaticMesh->SetCollisionResponseToChannel(MouseTraceChannel, ECR_Ignore);

                UE_LOG(OcclusionBinder, Log,
                    TEXT("AOcclusionBinder::SetupBoundActors>> Set ECR_Block on %s from %s"),
                    *StaticMesh->GetName(), *Actor->GetName());
            }
        }
    }

    UE_LOG(OcclusionBinder, Log,
        TEXT("AOcclusionBinder::SetupBoundActors>> Setup complete for %s | Bound actors: %d"),
        *GetName(), BoundActors.Num());
}

// ── Internal ──────────────────────────────────────────────────────────────────

void AOcclusionBinder::DiscoverAndRegisterMeshes()
{
    BoundMeshes.Reset();

    UOcclusionBinderSubsystem* Sub = GetWorld()->GetSubsystem<UOcclusionBinderSubsystem>();

    for (AActor* Actor : BoundActors)
    {
        if (!IsValid(Actor)) continue;

        TArray<UMeshComponent*> Meshes;
        Actor->GetComponents<UMeshComponent>(Meshes);

        for (UMeshComponent* Mesh : Meshes)
        {
            if (!Mesh) continue;

            BoundMeshes.Add(Mesh);

            // Register every primitive component so the subsystem can map
            // any hit primitive back to this binder
            if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Mesh))
            {
                if (Sub)
                    Sub->RegisterBinderPrimitive(Prim, this);
            }
        }

        UE_LOG(OcclusionBinder, Log,
            TEXT("AOcclusionBinder::DiscoverAndRegisterMeshes>> %s | Found %d meshes on %s"),
            *GetName(), Meshes.Num(), *Actor->GetName());
    }

    UE_LOG(OcclusionBinder, Log,
        TEXT("AOcclusionBinder::DiscoverAndRegisterMeshes>> %s | Total meshes: %d"),
        *GetName(), BoundMeshes.Num());
}

void AOcclusionBinder::InitializeMaterials()
{
    DynamicMaterials.Reset();

    for (TWeakObjectPtr<UMeshComponent> MeshPtr : BoundMeshes)
    {
        UMeshComponent* Mesh = MeshPtr.Get();
        if (!Mesh) continue;

        for (int32 i = 0; i < Mesh->GetNumMaterials(); ++i)
        {
            UMaterialInterface* Mat = Mesh->GetMaterial(i);
            if (!Mat) continue;

            UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Mat, this);
            if (!MID) continue;

            Mesh->SetMaterial(i, MID);
            DynamicMaterials.Add(MID);
        }
    }

    UE_LOG(OcclusionBinder, Log,
        TEXT("AOcclusionBinder::InitializeMaterials>> %s | Created %d MIDs"),
        *GetName(), DynamicMaterials.Num());
}

void AOcclusionBinder::UpdateMaterialAlpha()
{
    for (UMaterialInstanceDynamic* MID : DynamicMaterials)
    {
        if (MID) MID->SetScalarParameterValue(AlphaParameterName, CurrentAlpha);
    }
}

void AOcclusionBinder::CleanupInvalidOverlaps()
{
    for (auto It = ActiveOverlaps.CreateIterator(); It; ++It)
    {
        if (!It->IsValid()) It.RemoveCurrent();
    }
}