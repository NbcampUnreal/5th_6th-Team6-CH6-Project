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

    if (MathType != ECurveMathType::ZHeightOnly)
    {
        UE_LOG(LogTemp, Warning, TEXT("VisualCurvedToWorld: Only exact for ZHeightOnly for now"));
    }

    // Remove Z before computing camera space
    FVector FlatVisual = VisualPos;
    FlatVisual.Z = CurvedWorld->Camera_Origin.Z; // important

    FVector Offset = FlatVisual - CurvedWorld->Camera_Origin;

    FVector Right = CurvedWorld->Camera_RightVector.GetSafeNormal();
    FVector Forward = CurvedWorld->Camera_ForwardVector.GetSafeNormal();

    float OffsetX = FVector::DotProduct(Offset, Right);
    float OffsetY = FVector::DotProduct(Offset, Forward);

    float Z_Bend_X = OffsetX * OffsetX * CurvedWorld->CurveX;
    float Z_Bend_Y = OffsetY * OffsetY * CurvedWorld->CurveY;

    float TotalZBend = (Z_Bend_X + Z_Bend_Y) * CurvedWorld->BendWeight;

    FVector WorldPos = VisualPos;
    WorldPos.Z -= TotalZBend;

    return WorldPos;
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
   UE_LOG(LogTemp, Warning, TEXT("===== CURVED HIT START ====="));

    if (!PlayerController || !CurvedWorld)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid PlayerController or CurvedWorld"));
        return false;
    }

    if (MathType != ECurveMathType::ZHeightOnly)
    {
        UE_LOG(LogTemp, Error, TEXT("Wrong MathType"));
        return false;
    }

    FVector RayOrigin;
    FVector RayDir;

    if (!PlayerController->DeprojectMousePositionToWorld(RayOrigin, RayDir))
    {
        UE_LOG(LogTemp, Error, TEXT("Deproject failed"));
        return false;
    }

    RayDir.Normalize();

    UE_LOG(LogTemp, Warning, TEXT("RayOrigin: %s"), *RayOrigin.ToString());
    UE_LOG(LogTemp, Warning, TEXT("RayDir: %s"), *RayDir.ToString());

    // Subsystem values
    const FVector Origin   = CurvedWorld->Camera_Origin;
    const FVector Right    = CurvedWorld->Camera_RightVector.GetSafeNormal();
    const FVector Forward  = CurvedWorld->Camera_ForwardVector.GetSafeNormal();

    const float CurveX     = CurvedWorld->CurveX;
    const float CurveY     = CurvedWorld->CurveY;
    const float BendWeight = CurvedWorld->BendWeight;

    const float GroundZ = 0.f;

    UE_LOG(LogTemp, Warning, TEXT("Origin: %s"), *Origin.ToString());
    UE_LOG(LogTemp, Warning, TEXT("CurveX: %f CurveY: %f BendWeight: %f"),
        CurveX, CurveY, BendWeight);

    // Ray relative to curve origin
    const FVector O = RayOrigin - Origin;
    const FVector D = RayDir;

    const float Or = FVector::DotProduct(FVector(O.X, O.Y, 0.f), FVector(Right.X, Right.Y, 0.f));
    const float Of = FVector::DotProduct(FVector(O.X, O.Y, 0.f), FVector(Forward.X, Forward.Y, 0.f));

    const float Dr = FVector::DotProduct(FVector(D.X, D.Y, 0.f), FVector(Right.X, Right.Y, 0.f));
    const float Df = FVector::DotProduct(FVector(D.X, D.Y, 0.f), FVector(Forward.X, Forward.Y, 0.f));

    const float dz = D.Z;
    const float oz = RayOrigin.Z;

    UE_LOG(LogTemp, Warning, TEXT("Or: %f  Of: %f"), Or, Of);
    UE_LOG(LogTemp, Warning, TEXT("Dr: %f  Df: %f"), Dr, Df);
    UE_LOG(LogTemp, Warning, TEXT("oz: %f  dz: %f"), oz, dz);

    const float A =
        -BendWeight * (
            CurveX * Dr * Dr +
            CurveY * Df * Df
        );

    const float B =
        dz
        - 2.f * BendWeight * (
            CurveX * Or * Dr +
            CurveY * Of * Df
        );

    const float C =
        oz - GroundZ
        - BendWeight * (
            CurveX * Or * Or +
            CurveY * Of * Of
        );

    UE_LOG(LogTemp, Warning, TEXT("A: %f  B: %f  C: %f"), A, B, C);

    float t = -1.f;

    if (FMath::IsNearlyZero(A))
    {
        UE_LOG(LogTemp, Warning, TEXT("A nearly zero (linear case)"));

        if (FMath::IsNearlyZero(B))
        {
            UE_LOG(LogTemp, Error, TEXT("Both A and B nearly zero -> no solution"));
            return false;
        }

        t = -C / B;
    }
    else
    {
        const float Discriminant = B * B - 4.f * A * C;

        UE_LOG(LogTemp, Warning, TEXT("Discriminant: %f"), Discriminant);

        if (Discriminant < 0.f)
        {
            UE_LOG(LogTemp, Error, TEXT("Discriminant < 0 -> no intersection"));
            return false;
        }

        const float SqrtD = FMath::Sqrt(Discriminant);

        const float t1 = (-B + SqrtD) / (2.f * A);
        const float t2 = (-B - SqrtD) / (2.f * A);

        UE_LOG(LogTemp, Warning, TEXT("t1: %f  t2: %f"), t1, t2);

        t = TNumericLimits<float>::Max();

        if (t1 > 0.f) t = t1;
        if (t2 > 0.f && t2 < t) t = t2;

        if (t == TNumericLimits<float>::Max())
        {
            UE_LOG(LogTemp, Error, TEXT("No positive t found"));
            return false;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Chosen t: %f"), t);

    if (t <= 0.f || !FMath::IsFinite(t))
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid t"));
        return false;
    }

    const FVector CurvedHit = RayOrigin + RayDir * t;

    UE_LOG(LogTemp, Warning, TEXT("CurvedHit: %s"), *CurvedHit.ToString());

    // Verification trace
    UWorld* World = PlayerController->GetWorld();

    if (World)
    {
        FHitResult VerificationHit;

        FCollisionQueryParams Params(SCENE_QUERY_STAT(CurvedVerify), true);
        Params.AddIgnoredActor(PlayerController->GetPawn());

        bool bHit = World->LineTraceSingleByChannel(
            VerificationHit,
            RayOrigin,
            CurvedHit,
            TraceChannel,
            Params);

        UE_LOG(LogTemp, Warning, TEXT("Verification Trace Result: %s"),
            bHit ? TEXT("HIT") : TEXT("MISS"));

        if (!bHit)
            return false;
    }

    OutHitResult = FHitResult();
    OutHitResult.Location = CurvedHit;
    OutHitResult.ImpactPoint = CurvedHit;
    OutHitResult.bBlockingHit = true;

    UE_LOG(LogTemp, Warning, TEXT("===== CURVED HIT SUCCESS ====="));

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