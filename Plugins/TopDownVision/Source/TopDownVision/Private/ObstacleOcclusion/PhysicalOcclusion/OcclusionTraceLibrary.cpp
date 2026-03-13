#include "TopDownVision/Public/ObstacleOcclusion/PhysicalOcclusion/OcclusionTraceLibrary.h"
#include "TopDownVision/Public/ObstacleOcclusion/PhysicalOcclusion/OcclusionObstacleComp_Physical.h"
#include "TopDownVision/Public/ObstacleOcclusion/OcclusionInterface.h"
#include "TopDownVisionDebug.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"

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
    Probe.HitSweepIndices.Reset();

    FCollisionQueryParams Params;
    Params.bTraceComplex = false;
    for (AActor* Ignored : IgnoredActors)
    {
        if (Ignored) Params.AddIgnoredActor(Ignored);
    }

    for (int32 SweepIdx = 0; SweepIdx < Probe.Sweeps.Num(); ++SweepIdx)
    {
        const FOcclusionSweepConfig& Sweep      = Probe.Sweeps[SweepIdx];
        const FVector                SweepOrigin = Probe.BaseOrigin + Sweep.OriginOffset;

        // Static overlap at position only — does NOT travel along a path
        // Prevents every sphere from hitting obstacles near the target
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
            DrawDebugLine(
                World,
                SweepOrigin,
                Probe.Target,
                LineColor,
                false,
                -1.f,
                0,
                2.f);
        }

        for (int32 i = 0; i < Overlaps.Num(); ++i)
        {
            AActor* HitActor = Overlaps[i].GetActor();
            if (!HitActor) continue;

            TArray<UOcclusionObstacleComp_Physical*> Comps;
            HitActor->GetComponents<UOcclusionObstacleComp_Physical>(Comps);

            if (bDebugDraw)
            {
                const FColor HitColor = Comps.Num() > 0 ? FColor::Orange : FColor::White;
                DrawDebugString(
                    World,
                    HitActor->GetActorLocation(),
                    FString::Printf(TEXT("%s [%s] Sphere[%d]"),
                        *HitActor->GetName(),
                        Comps.Num() > 0 ? TEXT("OCCLUSION") : TEXT("other"),
                        SweepIdx),
                    nullptr,
                    HitColor,
                    0.f,
                    true);
            }

            if (Comps.Num() == 0) continue;

            CurrentHits.Add(HitActor);
            Probe.HitSweepIndices.Add(SweepIdx);
        }
    }

    // Hit diff — notify enter for new hits, exit for dropped hits
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

    TArray<UOcclusionObstacleComp_Physical*> Comps;
    Actor->GetComponents<UOcclusionObstacleComp_Physical>(Comps);

    for (UOcclusionObstacleComp_Physical* Comp : Comps)
    {
        IOcclusionInterface::Execute_OnOcclusionEnter(Comp, TracerIdentity);
    }

    UE_LOG(Occlusion, Log,
        TEXT("FOcclusionTraceLibrary::NotifyEnter>> %s (%d comps)"),
        *Actor->GetName(), Comps.Num());
}

void FOcclusionTraceLibrary::NotifyExit(AActor* Actor, UObject* TracerIdentity)
{
    if (!Actor) return;

    TArray<UOcclusionObstacleComp_Physical*> Comps;
    Actor->GetComponents<UOcclusionObstacleComp_Physical>(Comps);

    for (UOcclusionObstacleComp_Physical* Comp : Comps)
    {
        IOcclusionInterface::Execute_OnOcclusionExit(Comp, TracerIdentity);
    }

    UE_LOG(Occlusion, Log,
        TEXT("FOcclusionTraceLibrary::NotifyExit>> %s (%d comps)"),
        *Actor->GetName(), Comps.Num());
}