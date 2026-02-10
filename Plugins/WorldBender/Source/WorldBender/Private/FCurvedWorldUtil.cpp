// Fill out your copyright notice in the Description page of Project Settings.


#include "FCurvedWorldUtil.h"


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
    
	// Project the World Offset onto the Camera's local horizontal axes
	float Offset_Camera_X =
		FVector::DotProduct(FVector(Offset.X, Offset.Y, 0.0f), FVector(RightVector.X, RightVector.Y, 0.0f));
	float Offset_Camera_Y =
		FVector::DotProduct(FVector(Offset.X, Offset.Y, 0.0f), FVector(ForwardVector.X, ForwardVector.Y, 0.0f));
    
	// Calculate the total Z displacement (Quadratic Curve)
	float Z_Bend_X = Offset_Camera_X * Offset_Camera_X * CurveX;
	float Z_Bend_Y = Offset_Camera_Y * Offset_Camera_Y * CurveY;
	float Total_Z_Bend = (Z_Bend_X + Z_Bend_Y) * BendWeight;
    
	// Return the World Position Offset vector
	return FVector(0.0f, 0.0f, Total_Z_Bend);
}
