#pragma once

#include "CoreMinimal.h"
#include "TopDownVision/Public/ObstacleOcclusion/PhysicalOcclusion/OcclusionTraceTypes.h"

class UWorld;
class AActor;

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

    static void NotifyEnter(AActor* Actor, UObject* TracerIdentity);
    static void NotifyExit(AActor* Actor, UObject* TracerIdentity);

    FOcclusionTraceLibrary() = delete;
};