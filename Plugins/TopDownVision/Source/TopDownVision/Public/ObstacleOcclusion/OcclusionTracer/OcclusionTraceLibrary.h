#pragma once

#include "CoreMinimal.h"
#include "TopDownVision/Public/ObstacleOcclusion/OcclusionTracer/OcclusionTraceTypes.h"

//FD
class UWorld;
class AActor;
class UPrimitiveComponent;

class TOPDOWNVISION_API FOcclusionTraceLibrary
{
public:

    static void RunProbe(
        FOcclusionProbe&        Probe,
        UWorld*                 World,
        const TArray<AActor*>&  IgnoredActors,
        UObject*                TracerIdentity,
        bool                    bDebugDraw = false);

    static void RunProbes(
        TArray<FOcclusionProbe>& Probes,
        UWorld*                  World,
        const TArray<AActor*>&   IgnoredActors,
        UObject*                 TracerIdentity,
        bool                     bDebugDraw = false);

    // ── Fibonacci cone line trace method (new) ────────────────────────────

    static void RunLineProbe(
        FOcclusionProbe&        Probe,
        UWorld*                 World,
        const TArray<AActor*>&  IgnoredActors,
        UObject*                TracerIdentity,
        bool                    bDebugDraw = false);

private:

    static void NotifyEnter(UActorComponent* Comp, UObject* TracerIdentity);
    static void NotifyExit(UActorComponent* Comp, UObject* TracerIdentity);

    static UActorComponent* FindOwningOcclusionComp(UPrimitiveComponent* HitPrimitive);

    FOcclusionTraceLibrary() = delete;
};