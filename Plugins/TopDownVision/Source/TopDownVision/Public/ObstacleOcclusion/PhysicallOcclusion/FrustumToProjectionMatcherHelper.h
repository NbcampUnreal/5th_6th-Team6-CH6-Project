// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

// FD
class UCameraComponent;
class APlayerCameraManager;

enum class ECameraProjectionType : uint8
{
    Perspective,
    Orthographic
};

struct FCameraFrustumParams
{
    float                  VerticalFOVDeg       = 90.f;
    float                  AspectRatio          = 1.777f;
    float                  FrustumHalfAngleRad  = 0.f;
    FVector                CameraLocation       = FVector::ZeroVector;
    FVector                CameraForward        = FVector::ForwardVector;
    ECameraProjectionType  ProjectionType       = ECameraProjectionType::Perspective;
};

class TOPDOWNVISION_API FFrustumProjectionMatcherHelper
{
public:

    // ── Extraction ────────────────────────────────────────────────────────

    static bool ExtractFromCameraComponent(
        const UCameraComponent*  CameraComponent,
        FCameraFrustumParams&    OutParams);

    static bool ExtractFromCameraManager(
        const APlayerCameraManager*  CameraManager,
        FCameraFrustumParams&        OutParams);

    // ── Computation ───────────────────────────────────────────────────────

    /** Precomputes tan(halfFOV) — stored into Params.FrustumHalfAngleRad */
    static float CalculateFrustumHalfAngleRad(
        const FCameraFrustumParams& Params);

    /** World-space sphere radius at SphereDepth that matches the screen footprint
     *  of TargetVisibleRadius at TargetDistance.
     *
     *  Perspective:   r = R * (SphereDepth / TargetDistance)
     *  Orthographic:  r = R  (no foreshortening)
     *
     *  Note: if SphereDepth > TargetDistance the radius will exceed
     *  TargetVisibleRadius — caller should keep SphereDepth <= TargetDistance. */
    static float CalculateSphereRadiusAtDepth(
        const FCameraFrustumParams& Params,
        float TargetDistance,
        float TargetVisibleRadius,
        float SphereDepth);

    /** Convert a normalized screen-space radius to a world-space radius at Depth.
     *  NormalizedScreenRadius is 0..1 where 1.0 = half the screen height.
     *  Requires FrustumHalfAngleRad to be populated (call CalculateFrustumHalfAngleRad first).
     *  Only meaningful for Perspective — returns 0 for Orthographic. */
    static float ScreenRadiusToWorldRadius(
        const FCameraFrustumParams& Params,
        float NormalizedScreenRadius,
        float Depth);
};