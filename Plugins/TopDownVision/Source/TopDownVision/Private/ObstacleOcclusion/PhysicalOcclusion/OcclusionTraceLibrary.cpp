#include "TopDownVision/Public/ObstacleOcclusion/PhysicalOcclusion/OcclusionTraceLibrary.h"
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

void FOcclusionTraceLibrary::RunLineProbe(
    FOcclusionProbe&        Probe,
    UWorld*                 World,
    const TArray<AActor*>&  IgnoredActors,
    UObject*                TracerIdentity,
    bool                    bDebugDraw)
{
    if (!World) return;

    TSet<TWeakObjectPtr<AActor>> CurrentHits;
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

                // Primary filter
                const float HitDepth = FVector::DotProduct(
                    FVector(H.ImpactPoint.X, H.ImpactPoint.Y, 0.f),
                    Probe.CameraForwardH);
                if (HitDepth >= PawnDepthOnCamAxis) return false;

                // Secondary sphere filter
                if (Probe.SecondaryFilterSphereRadius > 0.f)
                {
                    if (FVector::Dist(H.ImpactPoint, Probe.SecondaryFilterSphereCenter)
                        > Probe.SecondaryFilterSphereRadius) return false;
                }

                return H.GetActor()->GetComponentsByInterface(UOcclusionInterface::StaticClass()).Num() > 0;
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
            // Catches large meshes whose front face passes the depth filter
            // but sits nowhere near the pawn's actual screen area.
            if (Probe.SecondaryFilterSphereRadius > 0.f)
            {
                const float DistToSphereCenter = FVector::Dist(
                    Hit.ImpactPoint,
                    Probe.SecondaryFilterSphereCenter);

                if (DistToSphereCenter > Probe.SecondaryFilterSphereRadius) continue;
            }

            AActor* HitActor = Hit.GetActor();
            if (!HitActor) continue;

            TArray<UActorComponent*> Comps = HitActor->GetComponentsByInterface(UOcclusionInterface::StaticClass());

            if (bDebugDraw)
            {
                const FColor HitColor = Comps.Num() > 0 ? FColor::Orange : FColor::White;
                DrawDebugString(
                    World,
                    Hit.ImpactPoint,
                    FString::Printf(TEXT("%s [%s] Line[%d] HitD:%.0f PawnD:%.0f SphereD:%.0f/%.0f"),
                        *HitActor->GetName(),
                        Comps.Num() > 0 ? TEXT("OCCLUSION") : TEXT("other"),
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

            if (Comps.Num() == 0) continue;

            CurrentHits.Add(HitActor);
            Probe.HitLineIndices.Add(LineIdx);
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

            TArray<UActorComponent*> Comps = HitActor->GetComponentsByInterface(UOcclusionInterface::StaticClass());

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

    TArray<UActorComponent*> Comps = Actor->GetComponentsByInterface(UOcclusionInterface::StaticClass());

    for (UActorComponent* Comp : Comps)
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

    TArray<UActorComponent*> Comps = Actor->GetComponentsByInterface(UOcclusionInterface::StaticClass());

    for (UActorComponent* Comp : Comps)
    {
        IOcclusionInterface::Execute_OnOcclusionExit(Comp, TracerIdentity);
    }

    UE_LOG(Occlusion, Log,
        TEXT("FOcclusionTraceLibrary::NotifyExit>> %s (%d comps)"),
        *Actor->GetName(), Comps.Num());
}