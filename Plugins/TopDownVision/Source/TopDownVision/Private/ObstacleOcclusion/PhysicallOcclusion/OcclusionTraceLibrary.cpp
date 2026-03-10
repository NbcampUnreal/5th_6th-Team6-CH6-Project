#include "ObstacleOcclusion/PhysicallOcclusion/OcclusionTraceLibrary.h"
#include "ObstacleOcclusion/PhysicallOcclusion/OcclusionObstacleComponent.h"
#include "ObstacleOcclusion/PhysicallOcclusion/OcclusionInterface.h"
#include "TopDownVisionDebug.h"
#include "DrawDebugHelpers.h"

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

void FOcclusionTraceLibrary::RunProbe(
    FOcclusionProbe&        Probe,
    UWorld*                 World,
    const TArray<AActor*>&  IgnoredActors,
    UObject*                TracerIdentity,
    bool                    bDebugDraw)
{
    if (!World) return;

    TSet<TWeakObjectPtr<AActor>> CurrentHits;

    FCollisionQueryParams Params;
    for (AActor* Ignored : IgnoredActors)
    {
        if (Ignored) Params.AddIgnoredActor(Ignored);
    }

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
            DrawDebugLine(
                World,
                SweepOrigin,
                Probe.Target,
                LineColor,
                false,
                -1.f,
                0,
                2.f);

            for (const FHitResult& Hit : Hits)
            {
                DrawDebugSphere(
                    World,
                    Hit.ImpactPoint,
                    Sweep.SphereRadius,
                    8,
                    FColor::Orange,
                    false,
                    -1.f);
            }
        }

        for (const FHitResult& Hit : Hits)
        {
            AActor* HitActor = Hit.GetActor();
            if (!HitActor) continue;

            TArray<UOcclusionObstacleComponent*> Comps;
            HitActor->GetComponents<UOcclusionObstacleComponent>(Comps);
            if (Comps.Num() == 0) continue;

            CurrentHits.Add(HitActor);
        }
    }

    for (const TWeakObjectPtr<AActor>& Current : CurrentHits)
    {
        if (!Probe.PreviousHits.Contains(Current))
            NotifyEnter(Current.Get(), TracerIdentity);
    }

    for (const TWeakObjectPtr<AActor>& Previous : Probe.PreviousHits)
    {
        if (Previous.IsValid() && !CurrentHits.Contains(Previous))
            NotifyExit(Previous.Get(), TracerIdentity);
    }

    Probe.PreviousHits = CurrentHits;
}

void FOcclusionTraceLibrary::NotifyEnter(AActor* Actor, UObject* TracerIdentity)
{
    if (!Actor) return;

    TArray<UOcclusionObstacleComponent*> Comps;
    Actor->GetComponents<UOcclusionObstacleComponent>(Comps);

    for (UOcclusionObstacleComponent* Comp : Comps)
    {
        IOcclusionInterface::Execute_OnOcclusionEnter(Comp, TracerIdentity);
    }

    UE_LOG(Occlusion, Log, TEXT("FOcclusionTraceLibrary::NotifyEnter>> %s (%d comps)"),
        *Actor->GetName(), Comps.Num());
}

void FOcclusionTraceLibrary::NotifyExit(AActor* Actor, UObject* TracerIdentity)
{
    if (!Actor) return;

    TArray<UOcclusionObstacleComponent*> Comps;
    Actor->GetComponents<UOcclusionObstacleComponent>(Comps);

    for (UOcclusionObstacleComponent* Comp : Comps)
    {
        IOcclusionInterface::Execute_OnOcclusionExit(Comp, TracerIdentity);
    }

    UE_LOG(Occlusion, Log, TEXT("FOcclusionTraceLibrary::NotifyExit>> %s (%d comps)"),
        *Actor->GetName(), Comps.Num())
}