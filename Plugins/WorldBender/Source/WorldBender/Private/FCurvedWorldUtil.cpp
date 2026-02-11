// FCurvedWorldUtil.cpp
#include "FCurvedWorldUtil.h"
#include "CurvedWorldSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

//========== Wrapper Functions ==========//

FVector FCurvedWorldUtil::CalculateCurvedWorldOffset(
    const FVector& WorldPos,
    const UCurvedWorldSubsystem* CurvedWorld,
    ECurveMathType MathType)
{
    if (!CurvedWorld)
    {
        return FVector::ZeroVector;
    }

    switch (MathType)
    {
    case ECurveMathType::None:
        return FVector::ZeroVector;
        
    case ECurveMathType::ZHeightOnly:
        return Internal_CalculateOffset_Simple(
            WorldPos,
            CurvedWorld->Camera_Origin,
            CurvedWorld->CurveX,
            CurvedWorld->CurveY,
            CurvedWorld->BendWeight,
            CurvedWorld->Camera_RightVector,
            CurvedWorld->Camera_ForwardVector);
        
    case ECurveMathType::HeightTilt:
        return Internal_CalculateOffset_HeightTilt(
            WorldPos,
            CurvedWorld->Camera_Origin,
            CurvedWorld->CurveX,
            CurvedWorld->CurveY,
            CurvedWorld->BendWeight,
            CurvedWorld->Camera_RightVector,
            CurvedWorld->Camera_ForwardVector,
            CurvedWorld->Camera_UpVector);
        
    case ECurveMathType::ExponentialHeightTilt:
        return Internal_CalculateOffset_ExponentialTilt(
            WorldPos,
            CurvedWorld->Camera_Origin,
            CurvedWorld->CurveX,
            CurvedWorld->CurveY,
            CurvedWorld->BendWeight,
            CurvedWorld->Camera_RightVector,
            CurvedWorld->Camera_ForwardVector,
            CurvedWorld->Camera_UpVector,
            1.5f); // Default height power
        
    default:
        return FVector::ZeroVector;
    }
}

void FCurvedWorldUtil::CalculateCurvedWorldTransform(
    const FVector& WorldPos,
    const UCurvedWorldSubsystem* CurvedWorld,
    FVector& OutOffset,
    FRotator& OutRotation,
    ECurveMathType MathType)
{
    if (!CurvedWorld)
    {
        OutOffset = FVector::ZeroVector;
        OutRotation = FRotator::ZeroRotator;
        return;
    }

    switch (MathType)
    {
    case ECurveMathType::None:
        OutOffset = FVector::ZeroVector;
        OutRotation = FRotator::ZeroRotator;
        break;
        
    case ECurveMathType::ZHeightOnly:
        Internal_CalculateTransform_Simple(
            WorldPos,
            CurvedWorld->Camera_Origin,
            CurvedWorld->CurveX,
            CurvedWorld->CurveY,
            CurvedWorld->BendWeight,
            CurvedWorld->Camera_RightVector,
            CurvedWorld->Camera_ForwardVector,
            OutOffset,
            OutRotation);
        break;
        
    case ECurveMathType::HeightTilt:
        Internal_CalculateTransform_HeightTilt(
            WorldPos,
            CurvedWorld->Camera_Origin,
            CurvedWorld->CurveX,
            CurvedWorld->CurveY,
            CurvedWorld->BendWeight,
            CurvedWorld->Camera_RightVector,
            CurvedWorld->Camera_ForwardVector,
            CurvedWorld->Camera_UpVector,
            OutOffset,
            OutRotation);
        break;
        
    case ECurveMathType::ExponentialHeightTilt:
        Internal_CalculateTransform_ExponentialTilt(
            WorldPos,
            CurvedWorld->Camera_Origin,
            CurvedWorld->CurveX,
            CurvedWorld->CurveY,
            CurvedWorld->BendWeight,
            CurvedWorld->Camera_RightVector,
            CurvedWorld->Camera_ForwardVector,
            CurvedWorld->Camera_UpVector,
            OutOffset,
            OutRotation,
            1.5f);
        break;
        
    default:
        OutOffset = FVector::ZeroVector;
        OutRotation = FRotator::ZeroRotator;
        break;
    }
}

//========== Internal Implementation - Simple (Z Height Only) ==========//

FVector FCurvedWorldUtil::Internal_CalculateOffset_Simple(
    const FVector& WorldPos,
    const FVector& Origin,
    float CurveX,
    float CurveY,
    float BendWeight,
    const FVector& RightVector,
    const FVector& ForwardVector)
{
    // Calculate offset from origin
    FVector Offset = WorldPos - Origin;
    
    // Project the World Offset onto the Camera's local horizontal axes
    float Offset_Camera_X = FVector::DotProduct(FVector(Offset.X, Offset.Y, 0.0f), 
                                                 FVector(RightVector.X, RightVector.Y, 0.0f));
    
    float Offset_Camera_Y = FVector::DotProduct(FVector(Offset.X, Offset.Y, 0.0f), 
                                                 FVector(ForwardVector.X, ForwardVector.Y, 0.0f));
    
    // Calculate the total Z displacement (Quadratic Curve)
    float Z_Bend_X = Offset_Camera_X * Offset_Camera_X * CurveX;
    float Z_Bend_Y = Offset_Camera_Y * Offset_Camera_Y * CurveY;
    float Total_Z_Bend = (Z_Bend_X + Z_Bend_Y) * BendWeight;
    
    // Displacement is only along the World Z axis
    return FVector(0.0f, 0.0f, Total_Z_Bend);
}

void FCurvedWorldUtil::Internal_CalculateTransform_Simple(
    const FVector& WorldPos,
    const FVector& Origin,
    float CurveX,
    float CurveY,
    float BendWeight,
    const FVector& RightVector,
    const FVector& ForwardVector,
    FVector& OutOffset,
    FRotator& OutRotation)
{
    // Calculate offset from origin
    FVector Offset = WorldPos - Origin;
    
    float Offset_Camera_X = FVector::DotProduct(FVector(Offset.X, Offset.Y, 0.0f), 
                                                 FVector(RightVector.X, RightVector.Y, 0.0f));
    
    float Offset_Camera_Y = FVector::DotProduct(FVector(Offset.X, Offset.Y, 0.0f), 
                                                 FVector(ForwardVector.X, ForwardVector.Y, 0.0f));
    
    float Z_Bend_X = Offset_Camera_X * Offset_Camera_X * CurveX;
    float Z_Bend_Y = Offset_Camera_Y * Offset_Camera_Y * CurveY;
    float Total_Z_Bend = (Z_Bend_X + Z_Bend_Y) * BendWeight;
    
    OutOffset = FVector(0.0f, 0.0f, Total_Z_Bend);
    
    // Calculate rotation to align with curved surface
    float dZ_dX = 2.0f * Offset_Camera_X * CurveX * BendWeight;
    float dZ_dY = 2.0f * Offset_Camera_Y * CurveY * BendWeight;
    
    FVector TangentRight = RightVector + FVector::UpVector * dZ_dX;
    TangentRight.Normalize();
    
    FVector TangentForward = ForwardVector + FVector::UpVector * dZ_dY;
    TangentForward.Normalize();
    
    FVector SurfaceNormal = FVector::CrossProduct(TangentRight, TangentForward);
    SurfaceNormal.Normalize();
    
    FVector NewUp = SurfaceNormal;
    FVector NewForward = TangentForward;
    FVector NewRight = FVector::CrossProduct(NewForward, NewUp);
    NewRight.Normalize();
    NewForward = FVector::CrossProduct(NewUp, NewRight);
    NewForward.Normalize();
    
    FMatrix RotationMatrix = FMatrix(NewForward, NewRight, NewUp, FVector::ZeroVector);
    OutRotation = RotationMatrix.Rotator();
}

//========== Internal Implementation - Height Tilt ==========//

FVector FCurvedWorldUtil::Internal_CalculateOffset_HeightTilt(
    const FVector& WorldPos,
    const FVector& Origin,
    float CurveX,
    float CurveY,
    float BendWeight,
    const FVector& RightVector,
    const FVector& ForwardVector,
    const FVector& UpVector)
{
    FVector Offset = WorldPos - Origin;
    
    // Project onto camera axes
    float Offset_Camera_X = FVector::DotProduct(Offset, RightVector);
    float Offset_Camera_Y = FVector::DotProduct(Offset, ForwardVector);
    float Offset_Camera_Z = FVector::DotProduct(Offset, UpVector);
    
    // Height multiplier - higher vertices bend more
    float HeightMultiplier = 1.0f + (Offset_Camera_Z * 0.001f);
    
    // Base curve strength
    float BaseCurve_X = Offset_Camera_X * Offset_Camera_X * CurveX;
    float BaseCurve_Y = Offset_Camera_Y * Offset_Camera_Y * CurveY;
    
    // Z bending (vertical drop - scaled by height)
    float Z_Bend = (BaseCurve_X + BaseCurve_Y) * BendWeight * HeightMultiplier;
    
    // X bending (horizontal tilt - scaled by height)
    float X_Bend = -Offset_Camera_X * FMath::Abs(Offset_Camera_X) * CurveX * 0.5f * BendWeight * HeightMultiplier;
    
    // Y bending (forward tilt - scaled by height)
    float Y_Bend = -Offset_Camera_Y * FMath::Abs(Offset_Camera_Y) * CurveY * 0.5f * BendWeight * HeightMultiplier;
    
    // Combine into world-space offset
    return RightVector * X_Bend + ForwardVector * Y_Bend + UpVector * Z_Bend;
}

void FCurvedWorldUtil::Internal_CalculateTransform_HeightTilt(
    const FVector& WorldPos,
    const FVector& Origin,
    float CurveX,
    float CurveY,
    float BendWeight,
    const FVector& RightVector,
    const FVector& ForwardVector,
    const FVector& UpVector,
    FVector& OutOffset,
    FRotator& OutRotation)
{
    // Calculate offset
    OutOffset = Internal_CalculateOffset_HeightTilt(
        WorldPos, Origin, CurveX, CurveY, BendWeight, 
        RightVector, ForwardVector, UpVector);
    
    // Calculate rotation (simplified for now - can be enhanced)
    FVector Offset = WorldPos - Origin;
    float Offset_Camera_X = FVector::DotProduct(Offset, RightVector);
    float Offset_Camera_Y = FVector::DotProduct(Offset, ForwardVector);
    float Offset_Camera_Z = FVector::DotProduct(Offset, UpVector);
    
    float HeightMultiplier = 1.0f + (Offset_Camera_Z * 0.001f);
    
    float dZ_dX = 2.0f * Offset_Camera_X * CurveX * BendWeight * HeightMultiplier;
    float dZ_dY = 2.0f * Offset_Camera_Y * CurveY * BendWeight * HeightMultiplier;
    
    FVector TangentRight = RightVector + UpVector * dZ_dX;
    TangentRight.Normalize();
    
    FVector TangentForward = ForwardVector + UpVector * dZ_dY;
    TangentForward.Normalize();
    
    FVector SurfaceNormal = FVector::CrossProduct(TangentRight, TangentForward);
    SurfaceNormal.Normalize();
    
    FVector NewUp = SurfaceNormal;
    FVector NewForward = TangentForward;
    FVector NewRight = FVector::CrossProduct(NewForward, NewUp);
    NewRight.Normalize();
    NewForward = FVector::CrossProduct(NewUp, NewRight);
    NewForward.Normalize();
    
    FMatrix RotationMatrix = FMatrix(NewForward, NewRight, NewUp, FVector::ZeroVector);
    OutRotation = RotationMatrix.Rotator();
}

//========== Internal Implementation - Exponential Height Tilt ==========//

FVector FCurvedWorldUtil::Internal_CalculateOffset_ExponentialTilt(
    const FVector& WorldPos,
    const FVector& Origin,
    float CurveX,
    float CurveY,
    float BendWeight,
    const FVector& RightVector,
    const FVector& ForwardVector,
    const FVector& UpVector,
    float HeightPower)
{
    FVector Offset = WorldPos - Origin;
    
    float Offset_Camera_X = FVector::DotProduct(Offset, RightVector);
    float Offset_Camera_Y = FVector::DotProduct(Offset, ForwardVector);
    float Offset_Camera_Z = FVector::DotProduct(Offset, UpVector);
    
    // Exponential height multiplier
    float NormalizedHeight = FMath::Max(0.0f, Offset_Camera_Z * 0.001f);
    float HeightMultiplier = FMath::Pow(1.0f + NormalizedHeight, HeightPower);
    
    float BaseCurve_X = Offset_Camera_X * Offset_Camera_X * CurveX;
    float BaseCurve_Y = Offset_Camera_Y * Offset_Camera_Y * CurveY;
    
    float Z_Bend = (BaseCurve_X + BaseCurve_Y) * BendWeight * HeightMultiplier;
    float X_Bend = -Offset_Camera_X * FMath::Abs(Offset_Camera_X) * CurveX * 0.5f * BendWeight * HeightMultiplier;
    float Y_Bend = -Offset_Camera_Y * FMath::Abs(Offset_Camera_Y) * CurveY * 0.5f * BendWeight * HeightMultiplier;
    
    return RightVector * X_Bend + ForwardVector * Y_Bend + UpVector * Z_Bend;
}

void FCurvedWorldUtil::Internal_CalculateTransform_ExponentialTilt(
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
    float HeightPower)
{
    // Calculate offset
    OutOffset = Internal_CalculateOffset_ExponentialTilt(
        WorldPos, Origin, CurveX, CurveY, BendWeight, 
        RightVector, ForwardVector, UpVector, HeightPower);
    
    // Calculate rotation
    FVector Offset = WorldPos - Origin;
    float Offset_Camera_X = FVector::DotProduct(Offset, RightVector);
    float Offset_Camera_Y = FVector::DotProduct(Offset, ForwardVector);
    float Offset_Camera_Z = FVector::DotProduct(Offset, UpVector);
    
    float NormalizedHeight = FMath::Max(0.0f, Offset_Camera_Z * 0.001f);
    float HeightMultiplier = FMath::Pow(1.0f + NormalizedHeight, HeightPower);
    
    float dZ_dX = 2.0f * Offset_Camera_X * CurveX * BendWeight * HeightMultiplier;
    float dZ_dY = 2.0f * Offset_Camera_Y * CurveY * BendWeight * HeightMultiplier;
    
    FVector TangentRight = RightVector + UpVector * dZ_dX;
    TangentRight.Normalize();
    
    FVector TangentForward = ForwardVector + UpVector * dZ_dY;
    TangentForward.Normalize();
    
    FVector SurfaceNormal = FVector::CrossProduct(TangentRight, TangentForward);
    SurfaceNormal.Normalize();
    
    FVector NewUp = SurfaceNormal;
    FVector NewForward = TangentForward;
    FVector NewRight = FVector::CrossProduct(NewForward, NewUp);
    NewRight.Normalize();
    NewForward = FVector::CrossProduct(NewUp, NewRight);
    NewForward.Normalize();
    
    FMatrix RotationMatrix = FMatrix(NewForward, NewRight, NewUp, FVector::ZeroVector);
    OutRotation = RotationMatrix.Rotator();
}

//========== Projection Helper Functions ==========//

FVector FCurvedWorldUtil::VisualCurvedToWorld(
    const FVector& VisualPos,
    const UCurvedWorldSubsystem* CurvedWorld,
    ECurveMathType MathType)
{
    if (!CurvedWorld)
    {
        return VisualPos;
    }

    // !!! Inversion only works simply for ZHeightOnly mode
    // For Height Tilt modes, this is approximate
    if (MathType != ECurveMathType::ZHeightOnly)
    {
        UE_LOG(LogTemp, Warning, TEXT("VisualCurvedToWorld: Inversion only accurate for ZHeightOnly mode"));
    }
    
    // Calculate offset using visual position's X,Y components
    FVector Offset = VisualPos - CurvedWorld->Camera_Origin;
    
    float Offset_Camera_X = FVector::DotProduct(FVector(Offset.X, Offset.Y, 0.0f), 
                                                 FVector(CurvedWorld->Camera_RightVector.X, CurvedWorld->Camera_RightVector.Y, 0.0f));
    
    float Offset_Camera_Y = FVector::DotProduct(FVector(Offset.X, Offset.Y, 0.0f), 
                                                 FVector(CurvedWorld->Camera_ForwardVector.X, CurvedWorld->Camera_ForwardVector.Y, 0.0f));
    
    // Calculate Z bend
    float Z_Bend_X = Offset_Camera_X * Offset_Camera_X * CurvedWorld->CurveX;
    float Z_Bend_Y = Offset_Camera_Y * Offset_Camera_Y * CurvedWorld->CurveY;
    float Total_Z_Bend = (Z_Bend_X + Z_Bend_Y) * CurvedWorld->BendWeight;
    
    // Invert: WorldPos = VisualPos - CurvedOffset
    return FVector(VisualPos.X, VisualPos.Y, VisualPos.Z - Total_Z_Bend);
}

FVector FCurvedWorldUtil::WorldToVisualCurved(
    const FVector& WorldPos,
    const UCurvedWorldSubsystem* CurvedWorld,
    ECurveMathType MathType)
{
    if (!CurvedWorld)
    {
        return WorldPos;
    }
    
    FVector CurvedOffset = CalculateCurvedWorldOffset(WorldPos, CurvedWorld, MathType);
    return WorldPos + CurvedOffset;
}

bool FCurvedWorldUtil::GetHitResultUnderCursorCorrected(
    APlayerController* PlayerController,
    const UCurvedWorldSubsystem* CurvedWorld,
    FHitResult& OutHitResult,
    ECollisionChannel TraceChannel,
    ECurveMathType MathType)
{
    if (!PlayerController || !CurvedWorld)
    {
        return false;
    }

    UWorld* World = PlayerController->GetWorld();
    if (!World)
    {
        return false;
    }

    // Get mouse position
    float MouseX, MouseY;
    if (!PlayerController->GetMousePosition(MouseX, MouseY))
    {
        return false;
    }

    // Deproject screen position to world space
    FVector WorldLocation, WorldDirection;
    if (!PlayerController->DeprojectScreenPositionToWorld(MouseX, MouseY, WorldLocation, WorldDirection))
    {
        return false;
    }

    // Perform line trace
    FVector TraceStart = WorldLocation;
    FVector TraceEnd = WorldLocation + WorldDirection * 100000.0f;
    
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(PlayerController->GetPawn());
    
    bool bHit = World->LineTraceSingleByChannel(
        OutHitResult,
        TraceStart,
        TraceEnd,
        TraceChannel,
        QueryParams
    );
    
    if (!bHit)
    {
        return false;
    }
    
    // Convert hit location from visual curved space to world space
    FVector WorldHitPos = VisualCurvedToWorld(OutHitResult.Location, CurvedWorld, MathType);
    
    OutHitResult.Location = WorldHitPos;
    OutHitResult.ImpactPoint = WorldHitPos;
    
    return true;
}

TArray<FVector> FCurvedWorldUtil::GenerateCurvedPathPoints(
    const FVector& StartPos,
    const FVector& EndPos,
    int32 NumPoints,
    const UCurvedWorldSubsystem* CurvedWorld,
    ECurveMathType MathType)
{
    TArray<FVector> CurvedPoints;
    
    if (!CurvedWorld || NumPoints < 2)
    {
        return CurvedPoints;
    }
    
    CurvedPoints.Reserve(NumPoints);
    
    for (int32 i = 0; i < NumPoints; ++i)
    {
        float Alpha = static_cast<float>(i) / static_cast<float>(NumPoints - 1);
        FVector StraightPoint = FMath::Lerp(StartPos, EndPos, Alpha);
        
        // First point is camera origin - no offset needed
        if (i == 0 && StraightPoint.Equals(CurvedWorld->Camera_Origin, 0.1f))
        {
            CurvedPoints.Add(StartPos);
        }
        else
        {
            FVector CurvedOffset = CalculateCurvedWorldOffset(StraightPoint, CurvedWorld, MathType);
            FVector CurvedPoint = StraightPoint + CurvedOffset;
            CurvedPoints.Add(CurvedPoint);
        }
    }
    
    return CurvedPoints;
}