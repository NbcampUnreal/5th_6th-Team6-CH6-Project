#pragma once

#include "CoreMinimal.h"
#include "OcclusionTraceTypes.generated.h"

USTRUCT(BlueprintType)
struct FOcclusionSweepConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sweep")
    FVector OriginOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sweep")
    float SphereRadius = 20.f;
};

// Single line trace endpoint for the fibonacci cone method
USTRUCT(BlueprintType)
struct FOcclusionLineConfig
{
    GENERATED_BODY()

    // End point of the line — start is always camera location
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Line")
    FVector EndPoint = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FOcclusionProbe
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Probe")
    TArray<FOcclusionSweepConfig> Sweeps;

    // Line trace endpoints — populated when using fibonacci cone method
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Probe")
    TArray<FOcclusionLineConfig> Lines;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Probe")
    FVector BaseOrigin = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Probe")
    FVector Target = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Probe")
    TEnumAsByte<ECollisionChannel> Channel = ECC_GameTraceChannel1;

    // Camera horizontal forward — used as depth axis for behind-target filtering.
    // A vertical plane yaw-rotated toward the camera has this vector as its normal.
    // Set by tracer each frame before RunLineProbe is called.
    FVector CameraForwardH = FVector::ForwardVector;

    // Secondary sphere filter — frustum-sized sphere offset toward camera by half radius.
    // Rejects hits geometrically too far from the pawn to realistically occlude it.
    // Radius is derived from frustum projection at pawn depth — same as sphere array method.
    // Center is offset toward camera by half radius so sphere sits just in front of pawn.
    FVector SecondaryFilterSphereCenter = FVector::ZeroVector;
    float   SecondaryFilterSphereRadius = 0.f;

    // Runtime state — not serialized
    //TSet<TWeakObjectPtr<UActorComponent>> PreviousHits;

    TSet<TWeakObjectPtr<UObject>> PreviousHits;// to cover actor and actor comp -> change to uobject
    TSet<int32> HitSweepIndices;
    TSet<int32> HitLineIndices;
};