#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "TopDownVision/Public/ObstacleOcclusion/PhysicalOcclusion/FrustumToProjectionMatcherHelper.h"
#include "OcclusionProbeComp.generated.h"

/*
 * Used for detecting obstacles hiding the character on the camera view.
 * Made as a separate component — multiple actors may need to be shown in one camera.
 * Uses an array of sphere collisions placed along the camera-to-target line
 * with frustum-correct radii so each sphere matches the projected screen size.
 */

class USphereComponent;
class APlayerCameraManager;
class UCameraComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTER_API UOcclusionProbeComp : public USceneComponent
{
    GENERATED_BODY()

public:

    UOcclusionProbeComp();

    UFUNCTION(BlueprintCallable, Category="Occlusion Probe")
    void SetCameraManager(APlayerCameraManager* InCameraManager);

protected:

    virtual void BeginPlay() override;

public:

    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

private:

    void InitializeProbes();
    void UpdateProbes();
    bool RefreshFrustumParams();

private:

    // How many sphere probes to place along the line
    UPROPERTY(EditAnywhere, Category="Occlusion Probe")
    int32 MaxProbeCount = 12;

    // World units radius around the target that probes should not enter
    // Prevents probes from overlapping with the target's own collision
    UPROPERTY(EditAnywhere, Category="Occlusion Probe")
    float TargetVisibleRadius = 60.f;

    // Distance between each sphere = current radius * this value
    // 1.0 = spheres touch, 2.0 = one radius gap between each
    UPROPERTY(EditAnywhere, Category="Occlusion Probe", meta=(ClampMin=0.1f))
    float SweepGapRatio = 1.5f;

    // Rocket head 

    // World units from target where rocket head taper takes effect
    UPROPERTY(EditAnywhere, Category="Occlusion Probe|RocketHead", meta=(ClampMin=0.f))
    float RocketHeadDistance = 150.f;

    // Taper shape — 1.0 = linear, > 1.0 = concave (sharp tip), < 1.0 = convex (quick widen)
    UPROPERTY(EditAnywhere, Category="Occlusion Probe|RocketHead", meta=(ClampMin=0.1f))
    float RocketHeadExponent = 2.f;

    // Debug

    UPROPERTY(EditAnywhere, Category="Occlusion Probe|Debug")
    bool bDebugDraw = false;

    UPROPERTY(EditAnywhere, Category="Occlusion Probe")
    TEnumAsByte<ECollisionChannel> OcclusionProbeChannel = ECC_GameTraceChannel3;

    UPROPERTY()
    TArray<USphereComponent*> ProbeSpheres;

    UPROPERTY()
    APlayerCameraManager* CameraManager = nullptr;

    FCameraFrustumParams FrustumParams;
    bool bCameraReady = false;
};