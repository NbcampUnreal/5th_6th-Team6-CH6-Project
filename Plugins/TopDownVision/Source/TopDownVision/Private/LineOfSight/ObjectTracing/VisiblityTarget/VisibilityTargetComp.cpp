// Fill out your copyright notice in the Description page of Project Settings.


#include "LineOfSight/ObjectTracing/VisiblityTarget/VisibilityTargetComp.h"

#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"

UVisibilityTargetComp::UVisibilityTargetComp()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UVisibilityTargetComp::BeginPlay()
{
    Super::BeginPlay();

    // If TargetShapeComp was set before play (e.g. via SetTargetShapeComp in the
    // constructor or BeginPlay of the owner), bind overlap events now.
    if (TargetShapeComp)
    {
        TargetShapeComp->OnComponentBeginOverlap.AddDynamic(this, &UVisibilityTargetComp::OnVolumeOverlapBegin);
        TargetShapeComp->OnComponentEndOverlap.AddDynamic(this, &UVisibilityTargetComp::OnVolumeOverlapEnd);
    }
}

void UVisibilityTargetComp::SetTargetShapeComp(UPrimitiveComponent* NewShapeComp)
{
    // Unbind from the old comp if there was one
    if (TargetShapeComp)
    {
        TargetShapeComp->OnComponentBeginOverlap.RemoveDynamic(this, &UVisibilityTargetComp::OnVolumeOverlapBegin);
        TargetShapeComp->OnComponentEndOverlap.RemoveDynamic(this, &UVisibilityTargetComp::OnVolumeOverlapEnd);
    }

    TargetShapeComp = NewShapeComp;//set new comp

    if (TargetShapeComp && GetWorld() && GetWorld()->HasBegunPlay())
    {
        TargetShapeComp->OnComponentBeginOverlap.AddDynamic(this, &UVisibilityTargetComp::OnVolumeOverlapBegin);
        TargetShapeComp->OnComponentEndOverlap.AddDynamic(this, &UVisibilityTargetComp::OnVolumeOverlapEnd);
    }
}

bool UVisibilityTargetComp::IsFullyHiddenByVolumes() const
{
    if (LocalSamplePoints.IsEmpty() || ActiveVolumes.IsEmpty() || !TargetShapeComp)
    {
        return false;
    }

    const FTransform& WorldTransform = TargetShapeComp->GetComponentTransform();

    for (const FVector& LocalPt : LocalSamplePoints)
    {
        const FVector WorldPt = WorldTransform.TransformPosition(LocalPt);

        bool bCoveredByAny = false;
        for (UPrimitiveComponent* Volume : ActiveVolumes)
        {
            if (IsPointInsideVolume(WorldPt, Volume))
            {
                bCoveredByAny = true;
                break;
            }
        }

        // If even one sample point is not covered, the target is not fully hidden
        if (!bCoveredByAny)
        {
            return false;
        }
    }

    return true;
}

void UVisibilityTargetComp::BakeSamplePoints()
{
    if (!TargetShapeComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("UVisibilityTargetComp::BakeSamplePoints — TargetShapeComp is null on %s"),
            *GetOwner()->GetName());
        return;
    }

    LocalSamplePoints = SampleShapeLocal(TargetShapeComp);

    UE_LOG(LogTemp, Log, TEXT("UVisibilityTargetComp::BakeSamplePoints — baked %d points on %s"),
        LocalSamplePoints.Num(), *GetOwner()->GetName());
}

void UVisibilityTargetComp::OnVolumeOverlapBegin(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (OtherComp)
    {
        ActiveVolumes.Add(OtherComp);
    }
}

void UVisibilityTargetComp::OnVolumeOverlapEnd(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex)
{
    if (OtherComp)
    {
        ActiveVolumes.Remove(OtherComp);
    }
}


TArray<FVector> UVisibilityTargetComp::SampleShapeLocal(UPrimitiveComponent* ShapeComp) const
{
    if (const USphereComponent* Sphere = Cast<USphereComponent>(ShapeComp))
    {
        return SampleSphere(Sphere);
    }
    if (const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(ShapeComp))
    {
        return SampleCapsule(Capsule);
    }
    if (const UBoxComponent* Box = Cast<UBoxComponent>(ShapeComp))
    {
        return SampleBox(Box);
    }

    // Fallback: treat the bounding box as a box
    return SampleBounds(ShapeComp);
}


//  Sampler — Sphere
//  Uses a Fibonacci / golden-angle spiral for even surface coverage.

TArray<FVector> UVisibilityTargetComp::SampleSphere(const USphereComponent* Sphere) const
{
    TArray<FVector> Points;

    const float Radius = Sphere->GetUnscaledSphereRadius();

    // Estimate point count from surface area and spacing
    // Surface area of sphere = 4 * PI * r^2
    const float SurfaceArea = 4.f * PI * Radius * Radius;
    const int32 NumPoints = FMath::Max(8, FMath::RoundToInt(SurfaceArea / (SampleSpacing * SampleSpacing)));

    Points.Reserve(NumPoints);

    // Fibonacci sphere: golden angle in radians
    const float GoldenAngle = PI * (3.f - FMath::Sqrt(5.f)); // ~2.399 rad

    for (int32 i = 0; i < NumPoints; ++i)
    {
        // y goes from +1 to -1 across all points
        const float Y = 1.f - (2.f * i / FMath::Max(NumPoints - 1, 1));
        const float RadiusAtY = FMath::Sqrt(FMath::Max(0.f, 1.f - Y * Y));
        const float Theta = GoldenAngle * i;

        const float X = FMath::Cos(Theta) * RadiusAtY;
        const float Z = FMath::Sin(Theta) * RadiusAtY;

        Points.Add(FVector(X, Y, Z) * Radius);
    }

    return Points;
}


//  Sampler — Capsule
//  Top hemisphere + cylinder band rings + bottom hemisphere.

TArray<FVector> UVisibilityTargetComp::SampleCapsule(const UCapsuleComponent* Capsule) const
{
    TArray<FVector> Points;

    const float Radius     = Capsule->GetUnscaledCapsuleRadius();
    const float HalfHeight = Capsule->GetUnscaledCapsuleHalfHeight(); // includes hemisphere radii
    const float CylHalfHeight = HalfHeight - Radius;                  // pure cylinder portion

    // --- Cylinder band ---
    // Number of rings along cylinder height
    const int32 NumRings = FMath::Max(1, FMath::RoundToInt((CylHalfHeight * 2.f) / SampleSpacing));

    // Number of points per ring from circumference
    const float Circumference = 2.f * PI * Radius;
    const int32 PointsPerRing = FMath::Max(4, FMath::RoundToInt(Circumference / SampleSpacing));

    for (int32 Ring = 0; Ring <= NumRings; ++Ring)
    {
        // Z ranges from -CylHalfHeight to +CylHalfHeight
        const float Z = (NumRings == 0)
            ? 0.f
            : FMath::Lerp(-CylHalfHeight, CylHalfHeight, (float)Ring / NumRings);

        for (int32 j = 0; j < PointsPerRing; ++j)
        {
            const float Angle = (2.f * PI * j) / PointsPerRing;
            const float X = FMath::Cos(Angle) * Radius;
            const float Y = FMath::Sin(Angle) * Radius;
            Points.Add(FVector(X, Y, Z));
        }
    }

    // --- Hemispheres ---
    // Use Fibonacci hemisphere for top and bottom.
    // We take the full Fibonacci sphere point set and split on Z.

    const float HemiArea = 2.f * PI * Radius * Radius;
    const int32 HemiPoints = FMath::Max(4, FMath::RoundToInt(HemiArea / (SampleSpacing * SampleSpacing)));

    const float GoldenAngle = PI * (3.f - FMath::Sqrt(5.f));

    // Full sphere sample, then split into upper / lower half
    const int32 TotalSpherePoints = HemiPoints * 2;

    for (int32 i = 0; i < TotalSpherePoints; ++i)
    {
        const float NormY = 1.f - (2.f * i / FMath::Max(TotalSpherePoints - 1, 1));
        const float R     = FMath::Sqrt(FMath::Max(0.f, 1.f - NormY * NormY));
        const float Theta = GoldenAngle * i;

        const FVector Unit(FMath::Cos(Theta) * R, FMath::Sin(Theta) * R, NormY);
        const FVector Scaled = Unit * Radius;

        if (Scaled.Z >= 0.f)
        {
            // Top hemisphere: offset upward by CylHalfHeight
            Points.Add(FVector(Scaled.X, Scaled.Y, Scaled.Z + CylHalfHeight));
        }
        else
        {
            // Bottom hemisphere: offset downward
            Points.Add(FVector(Scaled.X, Scaled.Y, Scaled.Z - CylHalfHeight));
        }
    }

    return Points;
}

//  Sampler — Box
//  Uniform grid on each of the 6 faces.

TArray<FVector> UVisibilityTargetComp::SampleBox(const UBoxComponent* Box) const
{
    TArray<FVector> Points;

    const FVector Extent = Box->GetUnscaledBoxExtent(); // half-extents

    // Helper lambda: sample one face as a 2D grid.
    // AxisU and AxisV are the two axes spanning the face.
    // FaceOffset is the constant offset along the face normal.
    auto SampleFace = [&](FVector FaceOffset, FVector AxisU, float HalfU, FVector AxisV, float HalfV)
    {
        const int32 StepsU = FMath::Max(1, FMath::RoundToInt((HalfU * 2.f) / SampleSpacing));
        const int32 StepsV = FMath::Max(1, FMath::RoundToInt((HalfV * 2.f) / SampleSpacing));

        for (int32 u = 0; u <= StepsU; ++u)
        {
            for (int32 v = 0; v <= StepsV; ++v)
            {
                const float U = FMath::Lerp(-HalfU, HalfU, (float)u / StepsU);
                const float V = FMath::Lerp(-HalfV, HalfV, (float)v / StepsV);
                Points.Add(FaceOffset + AxisU * U + AxisV * V);
            }
        }
    };

    // +X / -X faces (YZ plane)
    SampleFace( FVector( Extent.X, 0, 0), FVector(0,1,0), Extent.Y, FVector(0,0,1), Extent.Z);
    SampleFace( FVector(-Extent.X, 0, 0), FVector(0,1,0), Extent.Y, FVector(0,0,1), Extent.Z);

    // +Y / -Y faces (XZ plane)
    SampleFace( FVector(0,  Extent.Y, 0), FVector(1,0,0), Extent.X, FVector(0,0,1), Extent.Z);
    SampleFace( FVector(0, -Extent.Y, 0), FVector(1,0,0), Extent.X, FVector(0,0,1), Extent.Z);

    // +Z / -Z faces (XY plane)
    SampleFace( FVector(0, 0,  Extent.Z), FVector(1,0,0), Extent.X, FVector(0,1,0), Extent.Y);
    SampleFace( FVector(0, 0, -Extent.Z), FVector(1,0,0), Extent.X, FVector(0,1,0), Extent.Y);

    return Points;
}

//  Sampler — Bounds fallback
//  Treats the AABB as a box and delegates to SampleBox logic.

TArray<FVector> UVisibilityTargetComp::SampleBounds(const UPrimitiveComponent* Comp) const
{
    TArray<FVector> Points;

    // Get local-space bounding box
    const FBoxSphereBounds LocalBounds = Comp->CalcLocalBounds();
    const FVector Extent = LocalBounds.BoxExtent;
    const FVector Origin = LocalBounds.Origin; // usually zero, but respect it

    auto SampleFace = [&](FVector FaceOffset, FVector AxisU, float HalfU, FVector AxisV, float HalfV)
    {
        const int32 StepsU = FMath::Max(1, FMath::RoundToInt((HalfU * 2.f) / SampleSpacing));
        const int32 StepsV = FMath::Max(1, FMath::RoundToInt((HalfV * 2.f) / SampleSpacing));

        for (int32 u = 0; u <= StepsU; ++u)
        {
            for (int32 v = 0; v <= StepsV; ++v)
            {
                const float U = FMath::Lerp(-HalfU, HalfU, (float)u / StepsU);
                const float V = FMath::Lerp(-HalfV, HalfV, (float)v / StepsV);
                Points.Add(Origin + FaceOffset + AxisU * U + AxisV * V);
            }
        }
    };

    SampleFace( FVector( Extent.X, 0, 0), FVector(0,1,0), Extent.Y, FVector(0,0,1), Extent.Z);
    SampleFace( FVector(-Extent.X, 0, 0), FVector(0,1,0), Extent.Y, FVector(0,0,1), Extent.Z);
    SampleFace( FVector(0,  Extent.Y, 0), FVector(1,0,0), Extent.X, FVector(0,0,1), Extent.Z);
    SampleFace( FVector(0, -Extent.Y, 0), FVector(1,0,0), Extent.X, FVector(0,0,1), Extent.Z);
    SampleFace( FVector(0, 0,  Extent.Z), FVector(1,0,0), Extent.X, FVector(0,1,0), Extent.Y);
    SampleFace( FVector(0, 0, -Extent.Z), FVector(1,0,0), Extent.X, FVector(0,1,0), Extent.Y);

    return Points;
}

//  Volume containment helper

bool UVisibilityTargetComp::IsPointInsideVolume(const FVector& WorldPoint, UPrimitiveComponent* Volume) const
{
    if (!Volume)
    {
        return false;
    }

    // Transform the world point into the volume's local space
    const FTransform& VolumeTransform = Volume->GetComponentTransform();
    const FVector LocalPoint = VolumeTransform.InverseTransformPosition(WorldPoint);

    // --- Sphere ---
    if (const USphereComponent* Sphere = Cast<USphereComponent>(Volume))
    {
        return LocalPoint.SizeSquared() <= FMath::Square(Sphere->GetUnscaledSphereRadius());
    }

    // --- Box ---
    if (const UBoxComponent* Box = Cast<UBoxComponent>(Volume))
    {
        const FVector Extent = Box->GetUnscaledBoxExtent();
        return FMath::Abs(LocalPoint.X) <= Extent.X
            && FMath::Abs(LocalPoint.Y) <= Extent.Y
            && FMath::Abs(LocalPoint.Z) <= Extent.Z;
    }

    // --- Capsule ---
    if (const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Volume))
    {
        const float Radius     = Capsule->GetUnscaledCapsuleRadius();
        const float HalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
        const float CylHalf    = HalfHeight - Radius;

        // Clamp the point's Z to the cylinder segment, measure radial distance from that
        const float ClampedZ = FMath::Clamp(LocalPoint.Z, -CylHalf, CylHalf);
        const FVector ClosestOnAxis(0.f, 0.f, ClampedZ);
        return (LocalPoint - ClosestOnAxis).SizeSquared() <= FMath::Square(Radius);
    }

    // --- Fallback: AABB ---
    const FBoxSphereBounds LocalBounds = Volume->CalcLocalBounds();
    const FVector Extent = LocalBounds.BoxExtent;
    const FVector Origin = LocalBounds.Origin;
    const FVector Delta  = LocalPoint - Origin;

    return FMath::Abs(Delta.X) <= Extent.X
        && FMath::Abs(Delta.Y) <= Extent.Y
        && FMath::Abs(Delta.Z) <= Extent.Z;
}

