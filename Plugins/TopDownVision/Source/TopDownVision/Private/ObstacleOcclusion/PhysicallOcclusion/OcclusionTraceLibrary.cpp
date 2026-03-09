#include "ObstacleOcclusion/PhysicallOcclusion/OcclusionTraceLibrary.h"
#include "ObstacleOcclusion/PhysicallOcclusion/OcclusionObstacleComponent.h"
#include "ObstacleOcclusion/PhysicallOcclusion/OcclusionInterface.h"
#include "TopDownVisionDebug.h"
#include "DrawDebugHelpers.h"

void FOcclusionTraceLibrary::RunProbes(
    TArray<FOcclusionProbe>& Probes,
    UWorld* World,
    const TArray<AActor*>& IgnoredActors,
    bool bDebugDraw)
{
    for (FOcclusionProbe& Probe : Probes)
    {
        RunProbe(Probe, World, IgnoredActors, bDebugDraw);
    }
}

void FOcclusionTraceLibrary::RunProbe(
    FOcclusionProbe& Probe,
    UWorld* World,
    const TArray<AActor*>& IgnoredActors,
    bool bDebugDraw)
{
    if (!World) return;

    // Union of every sweep in this probe — one actor appears once even if
    // multiple sweeps hit it
    TSet<TWeakObjectPtr<AActor>> CurrentHits;

    FCollisionQueryParams Params;
    for (AActor* Ignored : IgnoredActors)
    {
        if (Ignored) Params.AddIgnoredActor(Ignored);
    }

    // ── Run every sweep and accumulate hits ───────────────────────────────
    for (const FOcclusionSweepConfig& Sweep : Probe.Sweeps)
    {
        const FVector SweepOrigin = Probe.BaseOrigin + Sweep.OriginOffset;

        TArray<FHitResult> Hits;
        World->SweepMultiByChannel(
            Hits,
            SweepOrigin,
            Probe.Target,
            FQuat::Identity,
            Probe.Channel,
            FCollisionShape::MakeSphere(Sweep.SphereRadius),
            Params);

        if (bDebugDraw)
        {
            const FColor LineColor = Hits.Num() > 0 ? FColor::Red : FColor::Green;
            DrawDebugLine(World, SweepOrigin, Probe.Target,
                          LineColor, false, -1.f, 0, 2.f);

            for (const FHitResult& Hit : Hits)
            {
                DrawDebugSphere(World, Hit.ImpactPoint,
                                Sweep.SphereRadius, 8, FColor::Orange, false, -1.f);
            }
        }

        for (const FHitResult& Hit : Hits)
        {
            AActor* HitActor = Hit.GetActor();
            if (!HitActor) continue;
            if (!HitActor->FindComponentByClass<UOcclusionObstacleComponent>()) continue;

            CurrentHits.Add(HitActor);
        }
    }

    // ── Enter: in CurrentHits but not in PreviousHits ─────────────────────
    for (const TWeakObjectPtr<AActor>& Current : CurrentHits)
    {
        if (!Probe.PreviousHits.Contains(Current))
        {
            NotifyEnter(Current.Get());
        }
    }

    // ── Exit: in PreviousHits but not in CurrentHits ──────────────────────
    for (const TWeakObjectPtr<AActor>& Previous : Probe.PreviousHits)
    {
        if (Previous.IsValid() && !CurrentHits.Contains(Previous))
        {
            NotifyExit(Previous.Get());
        }
    }

    Probe.PreviousHits = CurrentHits;
}

// ─────────────────────────────────────────────────────────────────────────────

void FOcclusionTraceLibrary::NotifyEnter(AActor* Actor)
{
    if (!Actor) return;
    IOcclusionInterface::Execute_OnOcclusionEnter(Actor, nullptr);
    UE_LOG(Occlusion, Log, TEXT("OcclusionTraceLibrary::NotifyEnter>> %s"), *Actor->GetName());
}

void FOcclusionTraceLibrary::NotifyExit(AActor* Actor)
{
    if (!Actor) return;
    IOcclusionInterface::Execute_OnOcclusionExit(Actor, nullptr);
    UE_LOG(Occlusion, Log, TEXT("OcclusionTraceLibrary::NotifyExit>> %s"), *Actor->GetName());
}
