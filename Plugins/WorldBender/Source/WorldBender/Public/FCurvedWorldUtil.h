// FCurvedWorldUtil.h
#pragma once

#include "CoreMinimal.h"
#include "CurvedWorldSubsystem.h"

/**
 *  This is for applying same math to the cpp as the material curve used for WPO
 */

UENUM(BlueprintType)
enum class ECurveMathType : uint8
{
    None UMETA(DisplayName="None"),
    
    ZHeightOnly UMETA(DisplayName="Simple(Z height only)"),
    HeightTilt UMETA(DisplayName="Tilt by height"),
    ExponentialHeightTilt UMETA(DisplayName="Tilt by Exponential height"),
};

class WORLDBENDER_API FCurvedWorldUtil
{
public:

    //========== Wrapper Functions (Use These) ==========//
    
    /**
     * Main curved world offset calculation - uses enum to determine method
     */
    static FVector CalculateCurvedWorldOffset(
        const FVector& WorldPos,
        const UCurvedWorldSubsystem* CurvedWorld,
        ECurveMathType MathType = ECurveMathType::ZHeightOnly);
    
    /**
     * Main curved world transform calculation - uses enum to determine method
     */
    static void CalculateCurvedWorldTransform(
        const FVector& WorldPos,
        const UCurvedWorldSubsystem* CurvedWorld,
        FVector& OutOffset,
        FRotator& OutRotation,
        ECurveMathType MathType = ECurveMathType::ZHeightOnly);

    //========== Internal Implementation Functions ==========//
private:
    
    // Simple version - Z height only
    static FVector Internal_CalculateOffset_Simple(
        const FVector& WorldPos,
        const FVector& Origin,
        float CurveX,
        float CurveY,
        float BendWeight,
        const FVector& RightVector,
        const FVector& ForwardVector);
    
    static void Internal_CalculateTransform_Simple(
        const FVector& WorldPos,
        const FVector& Origin,
        float CurveX,
        float CurveY,
        float BendWeight,
        const FVector& RightVector,
        const FVector& ForwardVector,
        FVector& OutOffset,
        FRotator& OutRotation);
    
    // Height tilt version
    static FVector Internal_CalculateOffset_HeightTilt(
        const FVector& WorldPos,
        const FVector& Origin,
        float CurveX,
        float CurveY,
        float BendWeight,
        const FVector& RightVector,
        const FVector& ForwardVector,
        const FVector& UpVector);
    
    static void Internal_CalculateTransform_HeightTilt(
        const FVector& WorldPos,
        const FVector& Origin,
        float CurveX,
        float CurveY,
        float BendWeight,
        const FVector& RightVector,
        const FVector& ForwardVector,
        const FVector& UpVector,
        FVector& OutOffset,
        FRotator& OutRotation);
    
    // Exponential height tilt version
    static FVector Internal_CalculateOffset_ExponentialTilt(
        const FVector& WorldPos,
        const FVector& Origin,
        float CurveX,
        float CurveY,
        float BendWeight,
        const FVector& RightVector,
        const FVector& ForwardVector,
        const FVector& UpVector,
        float HeightPower = 1.5f);
    
    static void Internal_CalculateTransform_ExponentialTilt(
        const FVector& WorldPos,
        const FVector& Origin,
        float CurveX,
        float CurveY,
        float BendWeight,
        const FVector& RightVector,
        const FVector& ForwardVector,
        const FVector& UpVector,
        FVector& OutOffset,
        FRotator& OutRotation,
        float HeightPower = 1.5f);

public:
    //========== Projection Helpers ==========//
    
    /**
     * Converts visual curved position to world space (inverse)
     */
    static FVector VisualCurvedToWorld(
        const FVector& VisualPos,
        const UCurvedWorldSubsystem* CurvedWorld,
        ECurveMathType MathType = ECurveMathType::ZHeightOnly);

    /**
     * Converts world position to visual curved position
     */
    static FVector WorldToVisualCurved(
        const FVector& WorldPos,
        const UCurvedWorldSubsystem* CurvedWorld,
        ECurveMathType MathType = ECurveMathType::ZHeightOnly);

    /**
     * Curved world aware cursor trace
     */
    static bool GetHitResultUnderCursorCorrected(
        APlayerController* PlayerController,
        const UCurvedWorldSubsystem* CurvedWorld,
        FHitResult& OutHitResult,
        ECollisionChannel TraceChannel,
        ECurveMathType MathType = ECurveMathType::ZHeightOnly);
    
    /**
     * Generate curved path points from start to end
     */
    static TArray<FVector> GenerateCurvedPathPoints(
        const FVector& StartPos,
        const FVector& EndPos,
        int32 NumPoints,
        const UCurvedWorldSubsystem* CurvedWorld,
        ECurveMathType MathType = ECurveMathType::ZHeightOnly);
};