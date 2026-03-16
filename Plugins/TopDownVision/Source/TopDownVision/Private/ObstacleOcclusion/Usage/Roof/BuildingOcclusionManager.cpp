#include "ObstacleOcclusion/Usage/Roof/BuildingOcclusionManager.h"
#include "ObstacleOcclusion/OcclusionTracer/OcclusionInterface.h"
#include "TopDownVisionDebug.h"

DEFINE_LOG_CATEGORY(BuildingOcclusionManager);

ABuildingOcclusionManager::ABuildingOcclusionManager()
{
    PrimaryActorTick.bCanEverTick = false;

    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void ABuildingOcclusionManager::OnInteriorEnter()
{
    if (bRoofCurrentlyOccluded) return;

    bRoofCurrentlyOccluded = true;
    ForceOccludeRoof(true);

    UE_LOG(BuildingOcclusionManager, Log,
        TEXT("ABuildingOcclusionManager::OnInteriorEnter>> %s | Roof occluded"),
        *GetName());
}

void ABuildingOcclusionManager::OnInteriorExit()
{
    if (!bRoofCurrentlyOccluded) return;

    bRoofCurrentlyOccluded = false;
    ForceOccludeRoof(false);

    UE_LOG(BuildingOcclusionManager, Log,
        TEXT("ABuildingOcclusionManager::OnInteriorExit>> %s | Roof released"),
        *GetName());
}

void ABuildingOcclusionManager::ForceOccludeRoof(bool bForce)
{
    for (AActor* Actor : RoofActors)
    {
        if (!IsValid(Actor)) continue;

        // AOcclusionBinder — implements interface directly on actor
        if (Actor->GetClass()->ImplementsInterface(UOcclusionInterface::StaticClass()))
        {
            IOcclusionInterface::Execute_ForceOcclude(Actor, bForce);
            continue;
        }

        // Comp-based — find interface comp on actor
        TArray<UActorComponent*> Comps = Actor->GetComponentsByInterface(UOcclusionInterface::StaticClass());
        for (UActorComponent* Comp : Comps)
        {
            if (Comp) IOcclusionInterface::Execute_ForceOcclude(Comp, bForce);
        }
    }
}