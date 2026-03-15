#include "TopDownVision/Public/ObstacleOcclusion/OcclusionTracer/OcclusionTraceLibrary.h"
#include "TopDownVision/Public/ObstacleOcclusion/OcclusionTracer/OcclusionInterface.h"
#include "TopDownVisionDebug.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"

// ── Helper ────────────────────────────────────────────────────────────────────

UActorComponent* FOcclusionTraceLibrary::FindOwningOcclusionComp(UPrimitiveComponent* HitPrimitive)
{
    if (!HitPrimitive) return nullptr;

    // Walk up the attachment chain from the hit primitive —
    // the first ancestor that implements IOcclusionInterface is the owning comp.
    // This correctly handles multi-comp actors by finding the specific comp
    // whose mesh subtree contains the hit primitive.
    USceneComponent* Current = HitPrimitive;
    while (Current)
    {
        if (Current->GetClass()->ImplementsInterface(UOcclusionInterface::StaticClass()))
            return Current;

        Current = Current->GetAttachParent();
    }

    // No occlusion comp found in attachment chain —
    // fall back to first interface comp on the actor.
    // This path fires when meshes are attached to the actor root rather than
    // directly to the occlusion comp — for full per-comp accuracy, meshes should
    // be children of their respective occlusion comp in the hierarchy.
    AActor* Owner = HitPrimitive->GetOwner();
    if (!Owner) return nullptr;

    TArray<UActorComponent*> Comps = Owner->GetComponentsByInterface(UOcclusionInterface::StaticClass());
    return Comps.Num() > 0 ? Comps[0] : nullptr;
}

// ── RunProbes ─────────────────────────────────────────────────────────────────

void FOcclusionTraceLibrary::RunProbes(
    TArray<FOcclusionProbe>& Probes,
    UWorld*                  World,
    const TArray<AActor*>&   IgnoredActors,
    UObject*                 TracerIdentity,
    bool                     bDebugDraw)
{
    for (FOcclusionProbe& Probe : Probes)
    {
        RunProbe(Probe, World, IgnoredActors, TracerIdentity, bDebugDraw);
    }
}

// ── RunProbe (sphere array) ───────────────────────────────────────────────────

void FOcclusionTraceLibrary::RunProbe(
    FOcclusionProbe&        Probe,
    UWorld*                 World,
    const TArray<AActor*>&  IgnoredActors,
    UObject*                TracerIdentity,
    bool                    bDebugDraw)
{
    if (!World) return;

    TSet<TWeakObjectPtr<UActorComponent>> CurrentHits;
    Probe.HitSweepIndices.Reset();

    FCollisionQueryParams Params;
    Params.bTraceComplex = false;
    for (AActor* Ignored : IgnoredActors)
    {
        if (Ignored) Params.AddIgnoredActor(Ignored);
    }

    for (int32 SweepIdx = 0; SweepIdx < Probe.Sweeps.Num(); ++SweepIdx)
    {
        const FOcclusionSweepConfig& Sweep       = Probe.Sweeps[SweepIdx];
        const FVector                SweepOrigin = Probe.BaseOrigin + Sweep.OriginOffset;

        TArray<FOverlapResult> Overlaps;
        World->OverlapMultiByChannel(
            Overlaps,
            SweepOrigin,
            FQuat::Identity,
            Probe.Channel,
            FCollisionShape::MakeSphere(Sweep.SphereRadius),
            Params);

        if (bDebugDraw)
        {
            const FColor LineColor = Overlaps.Num() > 0 ? FColor::Red : FColor::Green;
            DrawDebugLine(World, SweepOrigin, Probe.Target, LineColor, false, -1.f, 0, 2.f);
        }

        for (int32 i = 0; i < Overlaps.Num(); ++i)
        {
            UPrimitiveComponent* HitPrim = Overlaps[i].GetComponent();
            AActor*              HitActor = Overlaps[i].GetActor();
            if (!HitActor || !HitPrim) continue;

            // Find the specific occlusion comp that owns the hit primitive —
            // prevents multi-comp actors from all triggering on a single mesh hit
            UActorComponent* OcclusionComp = FindOwningOcclusionComp(HitPrim);

            if (bDebugDraw)
            {
                const FColor HitColor = OcclusionComp ? FColor::Orange : FColor::White;
                DrawDebugString(
                    World,
                    HitActor->GetActorLocation(),
                    FString::Printf(TEXT("%s [%s] Sphere[%d]"),
                        *HitActor->GetName(),
                        OcclusionComp ? TEXT("OCCLUSION") : TEXT("other"),
                        SweepIdx),
                    nullptr,
                    HitColor,
                    0.f,
                    true);
            }

            if (!OcclusionComp) continue;

            CurrentHits.Add(OcclusionComp);
            Probe.HitSweepIndices.Add(SweepIdx);
        }
    }

    for (const TWeakObjectPtr<UActorComponent>& Current : CurrentHits)
    {
        if (!Probe.PreviousHits.Contains(Current))
            NotifyEnter(Current.Get(), TracerIdentity);
    }

    for (const TWeakObjectPtr<UActorComponent>& Previous : Probe.PreviousHits)
    {
        if (Previous.IsValid() && !CurrentHits.Contains(Previous))
            NotifyExit(Previous.Get(), TracerIdentity);
    }

    Probe.PreviousHits = CurrentHits;
}

// ── RunLineProbe (fibonacci cone) ─────────────────────────────────────────────

void FOcclusionTraceLibrary::RunLineProbe(
    FOcclusionProbe&        Probe,
    UWorld*                 World,
    const TArray<AActor*>&  IgnoredActors,
    UObject*                TracerIdentity,
    bool                    bDebugDraw)
{
    if (!World) return;

    TSet<TWeakObjectPtr<UActorComponent>> CurrentHits;
    Probe.HitLineIndices.Reset();

    FCollisionQueryParams Params;
    Params.bTraceComplex = false;
    for (AActor* Ignored : IgnoredActors)
    {
        if (Ignored) Params.AddIgnoredActor(Ignored);
    }

    // Project pawn XY position onto camera horizontal forward axis —
    // absolute world position avoids drift with laterally offset cameras (e.g. 45 degree yaw rig).
    const float PawnDepthOnCamAxis = FVector::DotProduct(
        FVector(Probe.Target.X, Probe.Target.Y, 0.f),
        Probe.CameraForwardH);

    for (int32 LineIdx = 0; LineIdx < Probe.Lines.Num(); ++LineIdx)
    {
        const FVector& EndPoint = Probe.Lines[LineIdx].EndPoint;

        TArray<FHitResult> Hits;
        World->LineTraceMultiByChannel(
            Hits,
            Probe.BaseOrigin,
            EndPoint,
            Probe.Channel,
            Params);

        if (bDebugDraw)
        {
            const bool bAnyValid = Hits.ContainsByPredicate([&](const FHitResult& H)
            {
                if (H.Distance <= KINDA_SMALL_NUMBER) return false;
                if (!IsValid(H.GetActor())) return false;

                const float HitDepth = FVector::DotProduct(
                    FVector(H.ImpactPoint.X, H.ImpactPoint.Y, 0.f),
                    Probe.CameraForwardH);
                if (HitDepth >= PawnDepthOnCamAxis) return false;

                if (Probe.SecondaryFilterSphereRadius > 0.f)
                {
                    if (FVector::Dist(H.ImpactPoint, Probe.SecondaryFilterSphereCenter)
                        > Probe.SecondaryFilterSphereRadius) return false;
                }

                return FindOwningOcclusionComp(H.GetComponent()) != nullptr;
            });

            DrawDebugLine(
                World,
                Probe.BaseOrigin,
                EndPoint,
                bAnyValid ? FColor::Red : FColor::Green,
                false,
                -1.f,
                0,
                1.f);
        }

        for (const FHitResult& Hit : Hits)
        {
            // Ignore hits at or behind the camera
            if (Hit.Distance <= KINDA_SMALL_NUMBER) continue;

            // Primary filter — depth plane along camera horizontal forward axis.
            // Rejects anything at or past the pawn's absolute depth on the camera axis.
            const float HitDepth = FVector::DotProduct(
                FVector(Hit.ImpactPoint.X, Hit.ImpactPoint.Y, 0.f),
                Probe.CameraForwardH);

            if (HitDepth >= PawnDepthOnCamAxis) continue;

            // Secondary filter — frustum-sized sphere offset toward camera by half radius.
            // Rejects hits geometrically too far from the pawn to realistically occlude it.
            if (Probe.SecondaryFilterSphereRadius > 0.f)
            {
                const float DistToSphereCenter = FVector::Dist(
                    Hit.ImpactPoint,
                    Probe.SecondaryFilterSphereCenter);

                if (DistToSphereCenter > Probe.SecondaryFilterSphereRadius) continue;
            }

            UPrimitiveComponent* HitPrim  = Hit.GetComponent();
            AActor*              HitActor = Hit.GetActor();
            if (!HitActor || !HitPrim) continue;

            // Find the specific occlusion comp that owns the hit primitive
            UActorComponent* OcclusionComp = FindOwningOcclusionComp(HitPrim);

            if (bDebugDraw)
            {
                const FColor HitColor = OcclusionComp ? FColor::Orange : FColor::White;
                DrawDebugString(
                    World,
                    Hit.ImpactPoint,
                    FString::Printf(TEXT("%s [%s] Line[%d] HitD:%.0f PawnD:%.0f SphereD:%.0f/%.0f"),
                        *HitActor->GetName(),
                        OcclusionComp ? TEXT("OCCLUSION") : TEXT("other"),
                        LineIdx,
                        HitDepth,
                        PawnDepthOnCamAxis,
                        FVector::Dist(Hit.ImpactPoint, Probe.SecondaryFilterSphereCenter),
                        Probe.SecondaryFilterSphereRadius),
                    nullptr,
                    HitColor,
                    0.f,
                    true);
            }

            if (!OcclusionComp) continue;

            CurrentHits.Add(OcclusionComp);
            Probe.HitLineIndices.Add(LineIdx);
        }
    }

    // Hit diff — notify enter for new hits, exit for dropped hits
    for (const TWeakObjectPtr<UActorComponent>& Current : CurrentHits)
    {
        if (!Probe.PreviousHits.Contains(Current))
            NotifyEnter(Current.Get(), TracerIdentity);
    }

    for (const TWeakObjectPtr<UActorComponent>& Previous : Probe.PreviousHits)
    {
        if (Previous.IsValid() && !CurrentHits.Contains(Previous))
            NotifyExit(Previous.Get(), TracerIdentity);
    }

    Probe.PreviousHits = CurrentHits;
}

// ── Notify ────────────────────────────────────────────────────────────────────

void FOcclusionTraceLibrary::NotifyEnter(UActorComponent* Comp, UObject* TracerIdentity)
{
    if (!Comp) return;

    IOcclusionInterface::Execute_OnOcclusionEnter(Comp, TracerIdentity);

    UE_LOG(Occlusion, Log,
        TEXT("FOcclusionTraceLibrary::NotifyEnter>> %s on %s"),
        *Comp->GetName(),
        *Comp->GetOwner()->GetName());
}

void FOcclusionTraceLibrary::NotifyExit(UActorComponent* Comp, UObject* TracerIdentity)
{
    if (!Comp) return;

    IOcclusionInterface::Execute_OnOcclusionExit(Comp, TracerIdentity);

    UE_LOG(Occlusion, Log,
        TEXT("FOcclusionTraceLibrary::NotifyExit>> %s on %s"),
        *Comp->GetName(),
        *Comp->GetOwner()->GetName());
}