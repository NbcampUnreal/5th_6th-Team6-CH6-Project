// FCurvedWorldUtil.cpp
#include "FCurvedWorldUtil.h"
#include "CurvedWorldSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

void FCurvedWorldUtil::CalculateCurvedWorldTransform(const FVector& WorldPos, const FVector& Origin, float CurveX,
    float CurveY, float BendWeight, const FVector& RightVector, const FVector& ForwardVector, FVector& OutOffset,
    FRotator& OutRotation)
{
    // Calculate offset from origin
    FVector Offset = WorldPos - Origin;
    
    // Project the World Offset onto the Camera's local horizontal axes using the Dot Product
    // RightVector defines the camera's horizontal X-axis
    float Offset_Camera_X =
        FVector::DotProduct(FVector(Offset.X, Offset.Y, 0.0f), FVector(RightVector.X, RightVector.Y, 0.0f));
    
    // ForwardVector defines the camera's horizontal Y-axis
    float Offset_Camera_Y =
        FVector::DotProduct(FVector(Offset.X, Offset.Y, 0.0f), FVector(ForwardVector.X, ForwardVector.Y, 0.0f));
    
    // Calculate the total Z displacement based on Camera-Relative distance (Quadratic Curve)
    float Z_Bend_X = Offset_Camera_X * Offset_Camera_X * CurveX;
    float Z_Bend_Y = Offset_Camera_Y * Offset_Camera_Y * CurveY;
    float Total_Z_Bend = Z_Bend_X + Z_Bend_Y;
    
    // Apply the bending weight
    Total_Z_Bend *= BendWeight;
    
    // Create the World Position Offset vector (WPO)
    OutOffset = FVector(0.0f, 0.0f, Total_Z_Bend);
    
    // Calculate rotation to align with curved surface
    // We need the tangent vectors at this point on the curved surface
    
    // Derivative of Z with respect to camera X: dZ/dX = 2 * Offset_Camera_X * CurveX * BendWeight
    float dZ_dX = 2.0f * Offset_Camera_X * CurveX * BendWeight;
    
    // Derivative of Z with respect to camera Y: dZ/dY = 2 * Offset_Camera_Y * CurveY * BendWeight
    float dZ_dY = 2.0f * Offset_Camera_Y * CurveY * BendWeight;
    
    // Tangent in the Right direction (camera X)
    FVector TangentRight = RightVector + FVector::UpVector * dZ_dX;
    TangentRight.Normalize();
    
    // Tangent in the Forward direction (camera Y)
    FVector TangentForward = ForwardVector + FVector::UpVector * dZ_dY;
    TangentForward.Normalize();
    
    // Calculate the normal of the curved surface (cross product of tangents)
    FVector SurfaceNormal = FVector::CrossProduct(TangentRight, TangentForward);
    SurfaceNormal.Normalize();
    
    // Create rotation from the surface normal
    // We'll use the surface normal as the new "up" vector
    FVector NewUp = SurfaceNormal;
    FVector NewForward = TangentForward;
    FVector NewRight = FVector::CrossProduct(NewForward, NewUp);
    NewRight.Normalize();
    NewForward = FVector::CrossProduct(NewUp, NewRight);
    NewForward.Normalize();
    
    // Build rotation matrix and convert to rotator
    FMatrix RotationMatrix = FMatrix(NewForward, NewRight, NewUp, FVector::ZeroVector);
    OutRotation = RotationMatrix.Rotator();
}

FVector FCurvedWorldUtil::CalculateCurvedWorldOffset(const FVector& WorldPos, const FVector& Origin, float CurveX,
    float CurveY, float BendWeight, const FVector& RightVector, const FVector& ForwardVector)
{
    // Calculate offset from origin
    FVector Offset = WorldPos - Origin;
    
    // Project the World Offset onto the Camera's local horizontal axes using the Dot Product
    // RightVector defines the camera's horizontal X-axis
    float Offset_Camera_X = FVector::DotProduct(FVector(Offset.X, Offset.Y, 0.0f), 
                                                 FVector(RightVector.X, RightVector.Y, 0.0f));
    
    // ForwardVector defines the camera's horizontal Y-axis
    float Offset_Camera_Y = FVector::DotProduct(FVector(Offset.X, Offset.Y, 0.0f), 
                                                 FVector(ForwardVector.X, ForwardVector.Y, 0.0f));
    
    // Calculate the total Z displacement based on Camera-Relative distance (Quadratic Curve)
    float Z_Bend_X = Offset_Camera_X * Offset_Camera_X * CurveX;
    float Z_Bend_Y = Offset_Camera_Y * Offset_Camera_Y * CurveY;
    float Total_Z_Bend = (Z_Bend_X + Z_Bend_Y) * BendWeight;
    
    // Create the World Position Offset vector (WPO)
    // Displacement is only along the World Z axis
    return FVector(0.0f, 0.0f, Total_Z_Bend);
}

FVector FCurvedWorldUtil::VisualCurvedToWorld(
    const FVector& VisualPos,
    const UCurvedWorldSubsystem* CurvedWorld)
{
    if (!CurvedWorld)
    {
        return VisualPos;
    }
    
    // Calculate offset using visual position's X,Y components
    // Since the curve only affects Z, X and Y remain the same
    FVector Offset = VisualPos - CurvedWorld->Origin;
    
    float Offset_Camera_X = FVector::DotProduct(FVector(Offset.X, Offset.Y, 0.0f), 
                                                 FVector(CurvedWorld->RightVector.X, CurvedWorld->RightVector.Y, 0.0f));
    
    float Offset_Camera_Y = FVector::DotProduct(FVector(Offset.X, Offset.Y, 0.0f), 
                                                 FVector(CurvedWorld->ForwardVector.X, CurvedWorld->ForwardVector.Y, 0.0f));
    
    // Calculate Z bend
    float Z_Bend_X = Offset_Camera_X * Offset_Camera_X * CurvedWorld->CurveX;
    float Z_Bend_Y = Offset_Camera_Y * Offset_Camera_Y * CurvedWorld->CurveY;
    float Total_Z_Bend = (Z_Bend_X + Z_Bend_Y) * CurvedWorld->BendWeight;
    
    // Invert: WorldPos = VisualPos - CurvedOffset
    // X and Y are unchanged, only Z is affected
    return FVector(VisualPos.X, VisualPos.Y, VisualPos.Z - Total_Z_Bend);
}

FVector FCurvedWorldUtil::WorldToVisualCurved(
    const FVector& WorldPos,
    const UCurvedWorldSubsystem* CurvedWorld)
{
    if (!CurvedWorld)
    {
        return WorldPos;
    }
    
    FVector CurvedOffset = CalculateCurvedWorldOffset(
        WorldPos,
        CurvedWorld->Origin,
        CurvedWorld->CurveX,
        CurvedWorld->CurveY,
        CurvedWorld->BendWeight,
        CurvedWorld->RightVector,
        CurvedWorld->ForwardVector
    );
    
    return WorldPos + CurvedOffset;
}

bool FCurvedWorldUtil::GetHitResultUnderCursorCorrected(
    APlayerController* PlayerController,
    const UCurvedWorldSubsystem* CurvedWorld,
    FHitResult& OutHitResult,
    ECollisionChannel TraceChannel)
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
    FVector TraceEnd = WorldLocation + WorldDirection * 100000.0f; // Far distance
    
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
    
    // The hit location is in visual curved space
    // Convert it back to world space using direct inverse
    FVector WorldHitPos = VisualCurvedToWorld(OutHitResult.Location, CurvedWorld);
    
    // Update hit result with corrected world position
    OutHitResult.Location = WorldHitPos;
    OutHitResult.ImpactPoint = WorldHitPos;
    
    return true;
}

TArray<FVector> FCurvedWorldUtil::GenerateCurvedPathPoints(
    const FVector& StartPos,
    const FVector& EndPos,
    int32 NumPoints,
    const UCurvedWorldSubsystem* CurvedWorld)
{
    TArray<FVector> CurvedPoints;
    
    if (!CurvedWorld || NumPoints < 2)
    {
        return CurvedPoints;
    }
    
    CurvedPoints.Reserve(NumPoints);
    
    for (int32 i = 0; i < NumPoints; ++i)
    {
        // Interpolation factor (0.0 to 1.0)
        float Alpha = static_cast<float>(i) / static_cast<float>(NumPoints - 1);
        
        // Lerp along straight line
        FVector StraightPoint = FMath::Lerp(StartPos, EndPos, Alpha);
        
        // Apply curved world offset (camera position doesn't need offset)
        if (i == 0 && StraightPoint.Equals(CurvedWorld->Origin, 0.1f))
        {
            // First point is camera origin - no offset needed
            CurvedPoints.Add(StartPos);
        }
        else
        {
            FVector CurvedOffset = CalculateCurvedWorldOffset(
                StraightPoint,
                CurvedWorld->Origin,
                CurvedWorld->CurveX,
                CurvedWorld->CurveY,
                CurvedWorld->BendWeight,
                CurvedWorld->RightVector,
                CurvedWorld->ForwardVector
            );
            
            FVector CurvedPoint = StraightPoint + CurvedOffset;
            CurvedPoints.Add(CurvedPoint);
        }
    }
    
    return CurvedPoints;
}