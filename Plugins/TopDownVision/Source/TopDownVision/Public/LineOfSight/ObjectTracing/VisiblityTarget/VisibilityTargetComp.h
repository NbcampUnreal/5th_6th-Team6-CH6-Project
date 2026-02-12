// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VisibilityTargetComp.generated.h"

/*
 *  This will be used for self-evaluation,
 *  for checking if it is fully covered to the low obstacle volume or not to check if it is visible or not.
 *
 *  fuck
 */

// Forward declares
class USphereComponent;
class UCapsuleComponent;
class UBoxComponent;
class UPrimitiveComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UVisibilityTargetComp : public UActorComponent
{
    GENERATED_BODY()

public:
    UVisibilityTargetComp();

protected:
    virtual void BeginPlay() override;

public:
    // Setter for the shape component to sample from
    void SetTargetShapeComp(UPrimitiveComponent* NewShapeComp);

    // Returns true if all baked sample points (in world space) are inside active volumes
    bool IsFullyHiddenByVolumes() const;

    // Bakes local-space sample points from the current TargetShapeComp.
    // Can be called from the editor via CallInEditor.
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Visibility")
    void BakeSamplePoints();

protected:
    // --- Overlap bindings ---
    UFUNCTION()
    void OnVolumeOverlapBegin(
        UPrimitiveComponent* OverlappedComp,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

    UFUNCTION()
    void OnVolumeOverlapEnd(
        UPrimitiveComponent* OverlappedComp,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex);

private:
    // --- Sampler dispatcher ---
    // Detects the shape type and dispatches to the correct sampler.
    // Returns points in the component's local space.
    TArray<FVector> SampleShapeLocal(UPrimitiveComponent* ShapeComp) const;

    // --- Per-shape samplers (all return local-space points) ---
    TArray<FVector> SampleSphere(const USphereComponent* Sphere) const;
    TArray<FVector> SampleCapsule(const UCapsuleComponent* Capsule) const;
    TArray<FVector> SampleBox(const UBoxComponent* Box) const;
    TArray<FVector> SampleBounds(const UPrimitiveComponent* Comp) const; // fallback

    // --- Volume point-containment helper ---
    // Returns true if WorldPoint is inside the given primitive volume
    bool IsPointInsideVolume(const FVector& WorldPoint, UPrimitiveComponent* Volume) const;

    //===== Variables =====//
protected:
    // The primitive component whose surface will be sampled
    UPROPERTY(VisibleAnywhere, Category = "Visibility")
    UPrimitiveComponent* TargetShapeComp;

    // World-space distance between adjacent sample points
    UPROPERTY(EditAnywhere, Category = "Visibility", meta = (ClampMin = "1.0"))//clamp for safety
    float SampleSpacing = 20.f;

    // Baked sample points in local space — serialized so they survive editor sessions
    UPROPERTY(VisibleAnywhere, Category = "Visibility")
    TArray<FVector> LocalSamplePoints;

    // Currently overlapping low-obstacle volumes
    UPROPERTY(Transient)
    TSet<UPrimitiveComponent*> ActiveVolumes;

};
