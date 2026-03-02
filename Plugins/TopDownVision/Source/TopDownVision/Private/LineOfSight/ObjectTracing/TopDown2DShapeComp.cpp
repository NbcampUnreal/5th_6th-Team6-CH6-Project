#include "LineOfSight/ObjectTracing/TopDown2DShapeComp.h"
#include "Components/SplineComponent.h"

#include "DrawDebugHelpers.h"//Debug Draw

DEFINE_LOG_CATEGORY(TopDown2DShape);

UTopDown2DShapeComp::UTopDown2DShapeComp()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UTopDown2DShapeComp::SetSplineComp(USplineComponent* InSplineComp)
{
    ShapeSplineComp = InSplineComp;
}

void UTopDown2DShapeComp::GenerateSamplePoints()
{
    //reset Points
    InnerSamplePoints.Empty();
    OutLineSamplePoints.Empty();

    switch (ShapeType)
    {
    case E2DShapeType::Circle:
        GenerateInnerSamplePoints_Circle();
        GenerateOutlineSamplePoints_Circle();
        break;
    case E2DShapeType::Square:
        GenerateInnerSamplePoints_Square();
        GenerateOutlineSamplePoints_Square();
        break;
    case E2DShapeType::FreePoints:
        GenerateOutlineSamplePoints_FreePoints();
        GenerateInnerSamplePoints_FreePoints();
        break;
    default:
        UE_LOG(TopDown2DShape, Warning,
            TEXT("[%s] UTopDown2DShapeComp::GenerateSamplePoints >> ShapeType is None"),
            *GetOwner()->GetName());
        return;
    }

    UE_LOG(TopDown2DShape, Log,
        TEXT("[%s] GenerateSamplePoints >> Inner: %d, Outline: %d"),
        *GetOwner()->GetName(),
        InnerSamplePoints.Num(),
        OutLineSamplePoints.Num());
}

float UTopDown2DShapeComp::GetTraceRadius() const
{
    switch (ShapeType)
    {
    case E2DShapeType::Circle:
        return Radius;
    case E2DShapeType::Square:
        return FVector2D(BoxWidth * 0.5f, BoxHeight * 0.5f).Size(); // half-diagonal
    case E2DShapeType::FreePoints:
    {
        float MaxDist = 0.f;
        for (const FVector2D& P : OutLineSamplePoints)
            MaxDist = FMath::Max(MaxDist, P.Size());
        return MaxDist;
    }
    default:
        return 0.f;
    }
}

// -------------------------------------------------------------------------- //
// Circle
// -------------------------------------------------------------------------- //

void UTopDown2DShapeComp::DrawDebugShape(float Duration) const
{
    UWorld* World = GetWorld();
    if (!World)
        return;

    const FVector Origin     = GetComponentLocation();
    const FQuat   Rotation   = GetComponentQuat();
    const float   PointSize  = 4.f;
    const float   Z          = Origin.Z;

    // --- Draw shape boundary --- //
    switch (ShapeType)
    {
    case E2DShapeType::Circle:
        DrawDebugCircle(World, Origin, Radius,
            64, FColor::Cyan, false, Duration,
            0, 1.f,
            FVector(1,0,0), FVector(0,1,0)); // XY plane
        break;

    case E2DShapeType::Square:
    {
        const FVector Extent(BoxWidth * 0.5f, BoxHeight * 0.5f, 1.f);
        DrawDebugBox(
            World,
            Origin,
            Extent,
            Rotation,
            FColor::Cyan,
            false,
            Duration,
            0,
            1.f);
        break;
    }

    case E2DShapeType::FreePoints:
    {
        // Draw spline outline as connected line segments
        const int32 N = OutLineSamplePoints.Num();
        for (int32 i = 0; i < N; ++i)
        {
            const FVector2D& A2D = OutLineSamplePoints[i];
            const FVector2D& B2D = OutLineSamplePoints[(i + 1) % N];

            const FVector A = GetComponentTransform().TransformPosition(FVector(A2D.X, A2D.Y, 0.f));
            const FVector B = GetComponentTransform().TransformPosition(FVector(B2D.X, B2D.Y, 0.f));

            DrawDebugLine(
                World,
                A,
                B,
                FColor::Cyan,
                false,
                Duration,
                0,
                1.f);
        }
        break;
    }

    default:
        break;
    }

    // --- Draw inner sample points --- //
    for (const FVector2D& P : InnerSamplePoints)
    {
        const FVector WorldP = GetComponentTransform().TransformPosition(FVector(P.X, P.Y, 0.f));
        DrawDebugPoint(World, WorldP, PointSize, FColor::Green, false, Duration);
    }

    // --- Draw outline sample points --- //
    for (const FVector2D& P : OutLineSamplePoints)
    {
        const FVector WorldP = GetComponentTransform().TransformPosition(FVector(P.X, P.Y, 0.f));
        DrawDebugPoint(World, WorldP, PointSize, FColor::Yellow, false, Duration);
    }

    UE_LOG(TopDown2DShape, Verbose,
        TEXT("[%s] DrawDebugShape >> Shape=%d | Inner=%d | Outline=%d"),
        *GetOwner()->GetName(),
        (uint8)ShapeType,
        InnerSamplePoints.Num(),
        OutLineSamplePoints.Num());
}

void UTopDown2DShapeComp::GenerateInnerSamplePoints_Circle()
{
    // Fibonacci spiral — uniform inner coverage
    // Golden angle in radians
    const float GoldenAngle = PI * (3.f - FMath::Sqrt(5.f));
    const int32 NumPoints = FMath::FloorToInt((PI * Radius * Radius) / (PointSpacingDistance * PointSpacingDistance));

    for (int32 i = 0; i < NumPoints; ++i)
    {
        const float R = Radius * FMath::Sqrt((float)i / (float)NumPoints);
        const float Theta = GoldenAngle * i;
        InnerSamplePoints.Add(FVector2D(R * FMath::Cos(Theta), R * FMath::Sin(Theta)));
    }
}

void UTopDown2DShapeComp::GenerateOutlineSamplePoints_Circle()
{
    const float Circumference = 2.f * PI * Radius;
    const int32 NumPoints = FMath::Max(4, FMath::FloorToInt(Circumference / PointSpacingDistance));

    for (int32 i = 0; i < NumPoints; ++i)
    {
        const float Theta = (2.f * PI * i) / NumPoints;
        OutLineSamplePoints.Add(FVector2D(Radius * FMath::Cos(Theta), Radius * FMath::Sin(Theta)));
    }
}

// -------------------------------------------------------------------------- //
// Square
// -------------------------------------------------------------------------- //

void UTopDown2DShapeComp::GenerateInnerSamplePoints_Square()
{
    const float HalfW = BoxWidth  * 0.5f;
    const float HalfH = BoxHeight * 0.5f;

    for (float X = -HalfW; X <= HalfW; X += PointSpacingDistance)
    {
        for (float Y = -HalfH; Y <= HalfH; Y += PointSpacingDistance)
        {
            InnerSamplePoints.Add(FVector2D(X, Y));
        }
    }
}

void UTopDown2DShapeComp::GenerateOutlineSamplePoints_Square()
{
    const float HalfW = BoxWidth  * 0.5f;
    const float HalfH = BoxHeight * 0.5f;

    // Walk all 4 edges
    // Bottom
    for (float X = -HalfW; X <= HalfW; X += PointSpacingDistance)
        OutLineSamplePoints.Add(FVector2D(X, -HalfH));
    // Right
    for (float Y = -HalfH; Y <= HalfH; Y += PointSpacingDistance)
        OutLineSamplePoints.Add(FVector2D(HalfW, Y));
    // Top
    for (float X = HalfW; X >= -HalfW; X -= PointSpacingDistance)
        OutLineSamplePoints.Add(FVector2D(X, HalfH));
    // Left
    for (float Y = HalfH; Y >= -HalfH; Y -= PointSpacingDistance)
        OutLineSamplePoints.Add(FVector2D(-HalfW, Y));
}

// -------------------------------------------------------------------------- //
// FreePoints
// -------------------------------------------------------------------------- //

void UTopDown2DShapeComp::GenerateOutlineSamplePoints_FreePoints()
{
    if (!ShapeSplineComp)
        return;

    const float SplineLength = ShapeSplineComp->GetSplineLength();
    const int32 NumPoints = FMath::Max(4, FMath::FloorToInt(SplineLength / PointSpacingDistance));

    for (int32 i = 0; i < NumPoints; ++i)
    {
        const float Distance = SplineLength * i / NumPoints;
        const FVector WorldPos = ShapeSplineComp->GetLocationAtDistanceAlongSpline(
            Distance, ESplineCoordinateSpace::World);

        // Project to local 2D space of this component
        const FVector LocalPos = GetComponentTransform().InverseTransformPosition(WorldPos);
        OutLineSamplePoints.Add(FVector2D(LocalPos.X, LocalPos.Y));
    }
}

void UTopDown2DShapeComp::GenerateInnerSamplePoints_FreePoints()
{
    if (OutLineSamplePoints.IsEmpty())
        return;

    // Bounding box of the polygon
    FVector2D Min(FLT_MAX, FLT_MAX), Max(-FLT_MAX, -FLT_MAX);
    for (const FVector2D& P : OutLineSamplePoints)
    {
        Min.X = FMath::Min(Min.X, P.X);
        Min.Y = FMath::Min(Min.Y, P.Y);
        Max.X = FMath::Max(Max.X, P.X);
        Max.Y = FMath::Max(Max.Y, P.Y);
    }

    // Scanline grid fill — test each candidate against polygon
    for (float X = Min.X; X <= Max.X; X += PointSpacingDistance)
    {
        for (float Y = Min.Y; Y <= Max.Y; Y += PointSpacingDistance)
        {
            const FVector2D Candidate(X, Y);
            if (IsPointInsidePolygon(Candidate))
                InnerSamplePoints.Add(Candidate);
        }
    }
}

bool UTopDown2DShapeComp::IsPointInsidePolygon(const FVector2D& Point) const
{
    // Ray casting algorithm
    bool bInside = false;
    const int32 N = OutLineSamplePoints.Num();

    for (int32 i = 0, j = N - 1; i < N; j = i++)
    {
        const FVector2D& A = OutLineSamplePoints[i];
        const FVector2D& B = OutLineSamplePoints[j];

        if (((A.Y > Point.Y) != (B.Y > Point.Y)) &&
            (Point.X < (B.X - A.X) * (Point.Y - A.Y) / (B.Y - A.Y) + A.X))
        {
            bInside = !bInside;
        }
    }

    return bInside;
}