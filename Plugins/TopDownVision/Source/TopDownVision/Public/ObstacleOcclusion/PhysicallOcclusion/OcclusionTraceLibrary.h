#pragma once

#include "CoreMinimal.h"
#include "ObstacleOcclusion/PhysicallOcclusion/OcclusionTraceTypes.h"//required probe info

class UWorld;
class AActor;


class TOPDOWNVISION_API FOcclusionTraceLibrary
{
public:

    static void RunProbe(
        FOcclusionProbe&        Probe,
        UWorld*                 World,
        const TArray<AActor*>&  IgnoredActors,
        bool                    bDebugDraw = false);

    static void RunProbes(
        TArray<FOcclusionProbe>& Probes,
        UWorld*                  World,
        const TArray<AActor*>&   IgnoredActors,
        bool                     bDebugDraw = false);

private:

    static void NotifyEnter(AActor* Actor);
    static void NotifyExit(AActor* Actor);

    // Non-instantiable
    FOcclusionTraceLibrary() = delete;
};