// Fill out your copyright notice in the Description page of Project Settings.

#include "ObstacleOcclusion/OcclusionTracer/OcclusionTraceLibrary.h"

#include "ObstacleOcclusion/Binder/OcclusionBinder.h"
#include "TopDownVisionDebug.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "ObstacleOcclusion/Manager/OcclusionBinderSubsystem.h"

// ── Helper ────────────────────────────────────────────────────────────────────

UObject* FOcclusionTraceLibrary::FindOwningOcclusionObject(UPrimitiveComponent* HitPrimitive)
{
    if (!HitPrimitive) return nullptr;

    // 1. Walk attachment chain — mesh is child of occlusion comp
    USceneComponent* Current = HitPrimitive;
    while (Current)
    {
        if (Current->GetClass()->ImplementsInterface(UOcclusionInterface::StaticClass()))
            return Current;
        Current = Current->GetAttachParent();
    }

    // 2. GetComponentsByInterface on actor — comp lives on same actor as mesh
    AActor* Owner = HitPrimitive->GetOwner();
    if (!Owner) return nullptr;

    TArray<UActorComponent*> Comps = Owner->GetComponentsByInterface(UOcclusionInterface::StaticClass());
    if (Comps.Num() > 0) return Comps[0];

    // 3. Binder subsystem lookup — mesh belongs to a world-placed bound actor
    if (UWorld* World = HitPrimitive->GetWorld())
    {
        if (UOcclusionBinderSubsystem* Sub = World->GetSubsystem<UOcclusionBinderSubsystem>())
        {
            if (AOcclusionBinder* Binder = Sub->FindBinder(HitPrimitive))
                return Binder;
        }
    }

    return nullptr;
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

    TSet<TWeakObjectPtr<UObject>> CurrentHits;
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
            UPrimitiveComponent* HitPrim  = Overlaps[i].GetComponent();
            AActor*              HitActor = Overlaps[i].GetActor();
            if (!HitActor || !HitPrim) continue;

            UObject* OcclusionObject = FindOwningOcclusionObject(HitPrim);

            if (bDebugDraw)
            {
                const FColor HitColor = OcclusionObject ? FColor::Orange : FColor::White;
                DrawDebugString(
                    World,
                    HitActor->GetActorLocation(),
                    FString::Printf(TEXT("%s [%s] Sphere[%d]"),
                        *HitActor->GetName(),
                        OcclusionObject ? TEXT("OCCLUSION") : TEXT("other"),
                        SweepIdx),
                    nullptr,
                    HitColor,
                    0.f,
                    true);
            }

            if (!OcclusionObject) continue;

            CurrentHits.Add(OcclusionObject);
            Probe.HitSweepIndices.Add(SweepIdx);
        }
    }

    for (const TWeakObjectPtr<UObject>& Current : CurrentHits)
    {
        if (!Probe.PreviousHits.Contains(Current))
            NotifyEnter(Current.Get(), TracerIdentity);
    }

    for (const TWeakObjectPtr<UObject>& Previous : Probe.PreviousHits)
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

    TSet<TWeakObjectPtr<UObject>> CurrentHits;
    Probe.HitLineIndices.Reset();

    FCollisionQueryParams Params;
    Params.bTraceComplex = false;
    for (AActor* Ignored : IgnoredActors)
    {
        if (Ignored) Params.AddIgnoredActor(Ignored);
    }

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

                return FindOwningOcclusionObject(H.GetComponent()) != nullptr;
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
            if (Hit.Distance <= KINDA_SMALL_NUMBER) continue;

            const float HitDepth = FVector::DotProduct(
                FVector(Hit.ImpactPoint.X, Hit.ImpactPoint.Y, 0.f),
                Probe.CameraForwardH);

            if (HitDepth >= PawnDepthOnCamAxis) continue;

            if (Probe.SecondaryFilterSphereRadius > 0.f)
            {
                if (FVector::Dist(Hit.ImpactPoint, Probe.SecondaryFilterSphereCenter)
                    > Probe.SecondaryFilterSphereRadius) continue;
            }

            UPrimitiveComponent* HitPrim  = Hit.GetComponent();
            AActor*              HitActor = Hit.GetActor();
            if (!HitActor || !HitPrim) continue;

            UObject* OcclusionObject = FindOwningOcclusionObject(HitPrim);

            if (bDebugDraw)
            {
                const FColor HitColor = OcclusionObject ? FColor::Orange : FColor::White;
                DrawDebugString(
                    World,
                    Hit.ImpactPoint,
                    FString::Printf(TEXT("%s [%s] Line[%d] HitD:%.0f PawnD:%.0f SphereD:%.0f/%.0f"),
                        *HitActor->GetName(),
                        OcclusionObject ? TEXT("OCCLUSION") : TEXT("other"),
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

            if (!OcclusionObject) continue;

            CurrentHits.Add(OcclusionObject);
            Probe.HitLineIndices.Add(LineIdx);
        }
    }

    for (const TWeakObjectPtr<UObject>& Current : CurrentHits)
    {
        if (!Probe.PreviousHits.Contains(Current))
            NotifyEnter(Current.Get(), TracerIdentity);
    }

    for (const TWeakObjectPtr<UObject>& Previous : Probe.PreviousHits)
    {
        if (Previous.IsValid() && !CurrentHits.Contains(Previous))
            NotifyExit(Previous.Get(), TracerIdentity);
    }

    Probe.PreviousHits = CurrentHits;
}

// ── Notify ────────────────────────────────────────────────────────────────────

void FOcclusionTraceLibrary::NotifyEnter(UObject* OcclusionObject, UObject* TracerIdentity)
{
    if (!OcclusionObject) return;

    // Execute_OnOcclusionEnter works on any UObject implementing IOcclusionInterface —
    // covers UActorComponent (existing comps) and AActor (binder)
    IOcclusionInterface::Execute_OnOcclusionEnter(OcclusionObject, TracerIdentity);

    UE_LOG(Occlusion, Log,
        TEXT("FOcclusionTraceLibrary::NotifyEnter>> %s"),
        *OcclusionObject->GetName());
}

void FOcclusionTraceLibrary::NotifyExit(UObject* OcclusionObject, UObject* TracerIdentity)
{
    if (!OcclusionObject) return;

    IOcclusionInterface::Execute_OnOcclusionExit(OcclusionObject, TracerIdentity);

    UE_LOG(Occlusion, Log,
        TEXT("FOcclusionTraceLibrary::NotifyExit>> %s"),
        *OcclusionObject->GetName());
}