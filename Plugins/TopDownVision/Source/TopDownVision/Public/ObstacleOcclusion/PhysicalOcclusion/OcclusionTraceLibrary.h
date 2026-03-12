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

private:

    static void NotifyEnter(AActor* Actor, UObject* TracerIdentity);
    static void NotifyExit(AActor* Actor, UObject* TracerIdentity);

    static bool IsOccluderBetweenCameraAndTarget( // to filter the case when overlapped comp is behind the visible target
        const AActor*  HitActor,
        const FVector& CameraPos,
        const FVector& TargetPos);

    FOcclusionTraceLibrary() = delete;
};