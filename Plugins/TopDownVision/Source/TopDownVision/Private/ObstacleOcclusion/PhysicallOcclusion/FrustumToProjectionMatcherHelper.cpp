// Fill out your copyright notice in the Description page of Project Settings.

#include "ObstacleOcclusion/PhysicallOcclusion/FrustumToProjectionMatcherHelper.h"

#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"

// ── Extraction ────────────────────────────────────────────────────────────────

bool FFrustumProjectionMatcherHelper::ExtractFromCameraComponent(
    const UCameraComponent* CameraComponent,
    FCameraFrustumParams&   OutParams)
{
    if (!ensureMsgf(IsValid(CameraComponent),
        TEXT("ExtractFromCameraComponent: CameraComponent is null")))
    {
        return false;
    }

    OutParams.VerticalFOVDeg  = CameraComponent->FieldOfView;
    OutParams.AspectRatio     = CameraComponent->AspectRatio;
    OutParams.CameraLocation  = CameraComponent->GetComponentLocation();
    OutParams.CameraForward   = CameraComponent->GetForwardVector();
    OutParams.ProjectionType  = CameraComponent->ProjectionMode == ECameraProjectionMode::Perspective
                                    ? ECameraProjectionType::Perspective
                                    : ECameraProjectionType::Orthographic;

    OutParams.FrustumHalfAngleRad = CalculateFrustumHalfAngleRad(OutParams);

    return true;
}

bool FFrustumProjectionMatcherHelper::ExtractFromCameraManager(
    const APlayerCameraManager* CameraManager,
    FCameraFrustumParams&       OutParams)
{
    if (!ensureMsgf(IsValid(CameraManager),
        TEXT("ExtractFromCameraManager: CameraManager is null")))
    {
        return false;
    }

    OutParams.VerticalFOVDeg  = CameraManager->GetFOVAngle();
    OutParams.AspectRatio     = 16.f / 9.f; // CameraManager does not expose aspect ratio directly
    OutParams.CameraLocation  = CameraManager->GetCameraLocation();
    OutParams.CameraForward   = CameraManager->GetCameraRotation().Vector();
    OutParams.ProjectionType  = ECameraProjectionType::Perspective; // CameraManager always perspective

    OutParams.FrustumHalfAngleRad = CalculateFrustumHalfAngleRad(OutParams);

    return true;
}

// ── Computation ───────────────────────────────────────────────────────────────

float FFrustumProjectionMatcherHelper::CalculateFrustumHalfAngleRad(
    const FCameraFrustumParams& Params)
{
    return FMath::DegreesToRadians(Params.VerticalFOVDeg * 0.5f);
}

float FFrustumProjectionMatcherHelper::CalculateSphereRadiusAtDepth(
    const FCameraFrustumParams& Params,
    float TargetDistance,
    float TargetVisibleRadius,
    float SphereDepth)
{
    if (!ensureMsgf(TargetDistance > KINDA_SMALL_NUMBER,
        TEXT("CalculateSphereRadiusAtDepth: TargetDistance is zero or negative")))
    {
        return 0.f;
    }

    switch (Params.ProjectionType)
    {
        case ECameraProjectionType::Perspective:
            // r(d) = R * (d / D)
            // scales linearly with depth to preserve screen-space footprint
            return TargetVisibleRadius * (SphereDepth / TargetDistance);

        case ECameraProjectionType::Orthographic:
            // no foreshortening — radius is constant along the entire ray
            return TargetVisibleRadius;

        default:
            return 0.f;
    }
}

float FFrustumProjectionMatcherHelper::ScreenRadiusToWorldRadius(
    const FCameraFrustumParams& Params,
    float NormalizedScreenRadius,
    float Depth)
{
    if (!ensureMsgf(Depth > KINDA_SMALL_NUMBER,
        TEXT("ScreenRadiusToWorldRadius: Depth is zero or negative")))
    {
        return 0.f;
    }

    if (Params.ProjectionType == ECameraProjectionType::Orthographic)
    {
        // No depth scaling in orthographic — caller should use CalculateSphereRadiusAtDepth instead
        return 0.f;
    }

    // world_radius = tan(halfFOV) * Depth * NormalizedScreenRadius
    return FMath::Tan(Params.FrustumHalfAngleRad) * Depth * NormalizedScreenRadius;
}
