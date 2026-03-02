// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VolumeVisibilityEvaluatorComp.generated.h"

/*
 *  This will be used for self-evaluation,
 *  for checking if it is fully covered to the low obstacle volume or not to check if it is visible or not.
 *
 *  fuck
 *
 *   turned out this method is for 3d space, but current topdown project can work with 2d method.
 *   fuck
 */

// Forward declares
class USphereComponent;
class UCapsuleComponent;
class UBoxComponent;
class UPrimitiveComponent;

//Enum for shape

UENUM(BlueprintType)
enum class EShapeType : uint8
{
    None UMETA(DisplayName = "None"),
    
    Box UMETA(DisplayName = "Box"),
    Sphere UMETA(DisplayName = "Sphere"),
    Capsule UMETA(DisplayName = "Capsule"),
    Bounds UMETA(DisplayName = "Bounds"),
};

USTRUCT()
struct FVolumeShape //cachable object volume to compute if the sample point is in or out
{
    GENERATED_BODY()

    UPROPERTY()
    TWeakObjectPtr<UPrimitiveComponent> Component;

    UPROPERTY()
    EShapeType ShapeType = EShapeType::None;

    // Cached geometry
    float Radius = 0.f;// Sphere + Capsule
    float CapsuleCylHalf = 0.f;// Capsule only
    FVector BoxExtent = FVector::ZeroVector;// Box

    bool IsValid() const
    {
        return Component.IsValid();
    }
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UVolumeVisibilityEvaluatorComp : public UActorComponent
{
    GENERATED_BODY()

public:
    UVolumeVisibilityEvaluatorComp();

protected:
    virtual void BeginPlay() override;

public:
    // Setter for the shape component to sample from
    UFUNCTION(BlueprintCallable, Category = "Visibility")
    void SetTargetShapeComp(UPrimitiveComponent* NewShapeComp);

    // Returns true if all baked sample points (in world space) are inside active volumes
    UFUNCTION(BlueprintCallable, Category = "Visibility")
    bool IsFullyHiddenByVolumes() const;

    // Bakes local-space sample points from the current TargetShapeComp.
    // Can be called from the editor via CallInEditor.
    UFUNCTION(BlueprintCallable, Category = "Visibility")
    void BakeSamplePoints();

    UFUNCTION(BlueprintCallable, Category = "Visibility|Debug")
    void DebugDrawSamplePoints(float Duration = 10.f) const;

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
    
    //  Sampler — Sphere
    //  Uses a Fibonacci / golden-angle spiral for even surface coverage.
    TArray<FVector> SampleSphere(const USphereComponent* Sphere) const;
    
    //  Sampler — Capsule
    //  Top hemisphere + cylinder band rings + bottom hemisphere.
    TArray<FVector> SampleCapsule(const UCapsuleComponent* Capsule) const;
    
    //  Sampler — Box
    //  Uniform grid on each of the 6 faces.
    TArray<FVector> SampleBox(const UBoxComponent* Box) const;
    
    //  Sampler — Bounds fallback
    //  Treats the AABB as a box and delegates to SampleBox logic.
    TArray<FVector> SampleBounds(const UPrimitiveComponent* Comp) const; // fallback

    void SampleFace( //internal function for the box and bounds sampling
        TArray<FVector>& OutPoints,
        const FVector& FaceOffset,
        const FVector& AxisU, float HalfU,
        const FVector& AxisV, float HalfV,
        const FVector& Origin = FVector::ZeroVector) const;
    

    // --- Volume point-containment helper ---
    // Returns true if WorldPoint is inside the given primitive volume
    bool IsPointInsideVolume(
        const FVector& WorldPoint,
        const FVolumeShape& Volume) const;

    //===== Variables =====//
protected:
    UPROPERTY(EditAnywhere, Category = "Visibility")
    bool bDrawDebugSamplePoints=false;
    
    // The primitive component whose surface will be sampled
    UPROPERTY(VisibleAnywhere, Category = "Visibility")
    UPrimitiveComponent* TargetShapeComp=nullptr;

    // World-space distance between adjacent sample points
    UPROPERTY(EditAnywhere, Category = "Visibility", meta = (ClampMin = "1.0"))//clamp for safety
    float SampleSpacing = 20.f;

    // Baked sample points in local space — serialized so they survive editor sessions
    UPROPERTY(VisibleAnywhere, Category = "Visibility")
    TArray<FVector> LocalSamplePoints;

    // Currently overlapping low-obstacle volumes
    UPROPERTY(Transient)
    TArray<FVolumeShape> ActiveVolumes;

};
