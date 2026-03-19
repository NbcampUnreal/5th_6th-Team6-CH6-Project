#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BuildingOcclusionManager.generated.h"

TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(BuildingOcclusionManager, Log, All);

UCLASS()
class TOPDOWNVISION_API ABuildingOcclusionManager : public AActor
{
    GENERATED_BODY()

public:

    ABuildingOcclusionManager();

    // Called by UBuildingTrackerComponent when this manager's proxy is hit
    void OnInteriorEnter();

    // Called by UBuildingTrackerComponent when this manager's proxy is no longer hit
    void OnInteriorExit();

    // ── Config ────────────────────────────────────────────────────────────

    // All roof actors for this building — binders and comp-based actors
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Building Occlusion")
    TArray<TObjectPtr<AActor>> RoofActors;

private:

    void ForceOccludeRoof(bool bForce);

    bool bRoofCurrentlyOccluded = false;
};