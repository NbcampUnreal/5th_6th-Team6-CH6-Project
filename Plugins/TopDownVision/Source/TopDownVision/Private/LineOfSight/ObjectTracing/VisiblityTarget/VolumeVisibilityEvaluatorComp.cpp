// Fill out your copyright notice in the Description page of Project Settings.


#include "LineOfSight/ObjectTracing/VisiblityTarget/VolumeVisibilityEvaluatorComp.h"

#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"

#include "TopDownVisionDebug.h"//log
#include "DrawDebugHelpers.h"//debug draw

UVolumeVisibilityEvaluatorComp::UVolumeVisibilityEvaluatorComp()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UVolumeVisibilityEvaluatorComp::BeginPlay()
{
    Super::BeginPlay();

    const FString Ctx = TopDownVisionDebug::GetClientDebugName(this);

    if (TargetShapeComp)
    {
        TargetShapeComp->OnComponentBeginOverlap.AddDynamic(this, &UVolumeVisibilityEvaluatorComp::OnVolumeOverlapBegin);
        TargetShapeComp->OnComponentEndOverlap.AddDynamic(this, &UVolumeVisibilityEvaluatorComp::OnVolumeOverlapEnd);

        UE_LOG(LOSTrace, Log,
            TEXT("%s UVisibilityTargetComp::BeginPlay >> overlap bindings registered on [%s]"),
            *Ctx, *TargetShapeComp->GetName());
    }
    else
    {
        UE_LOG(LOSTrace, Warning,
            TEXT("%s UVisibilityTargetComp::BeginPlay >> TargetShapeComp is null — overlap bindings skipped"),
            *Ctx);
    }
}

void UVolumeVisibilityEvaluatorComp::SetTargetShapeComp(UPrimitiveComponent* NewShapeComp)
{
    const FString Ctx = TopDownVisionDebug::GetClientDebugName(this);

    // Unbind from the old comp if there was one
    if (TargetShapeComp)
    {
        TargetShapeComp->OnComponentBeginOverlap.RemoveDynamic(this, &UVolumeVisibilityEvaluatorComp::OnVolumeOverlapBegin);
        TargetShapeComp->OnComponentEndOverlap.RemoveDynamic(this, &UVolumeVisibilityEvaluatorComp::OnVolumeOverlapEnd);

        UE_LOG(LOSTrace, Log,
            TEXT("%s UVisibilityTargetComp::SetTargetShapeComp >> unbound from old comp [%s]"),
            *Ctx, *TargetShapeComp->GetName());
    }

    TargetShapeComp = NewShapeComp;

    if (TargetShapeComp)
    {
        UE_LOG(LOSTrace, Log,
            TEXT("%s UVisibilityTargetComp::SetTargetShapeComp >> assigned [%s]"),
            *Ctx, *TargetShapeComp->GetName());

        if (GetWorld() && GetWorld()->HasBegunPlay())
        {
            TargetShapeComp->OnComponentBeginOverlap.AddDynamic(this, &UVolumeVisibilityEvaluatorComp::OnVolumeOverlapBegin);
            TargetShapeComp->OnComponentEndOverlap.AddDynamic(this, &UVolumeVisibilityEvaluatorComp::OnVolumeOverlapEnd);

            UE_LOG(LOSTrace, Log,
                TEXT("%s UVisibilityTargetComp::SetTargetShapeComp >> overlap bindings registered (post-BeginPlay)"),
                *Ctx);
        }
    }
    else
    {
        UE_LOG(LOSTrace, Warning,
            TEXT("%s UVisibilityTargetComp::SetTargetShapeComp >> NewShapeComp is null"),
            *Ctx);
    }
}

bool UVolumeVisibilityEvaluatorComp::IsFullyHiddenByVolumes() const
{
    const FString Ctx = TopDownVisionDebug::GetClientDebugName(this);

    if (!TargetShapeComp)
        return false;

    if (LocalSamplePoints.IsEmpty())
        return false;

    if (ActiveVolumes.IsEmpty())
        return false;

    const FTransform& WorldTransform = TargetShapeComp->GetComponentTransform();

    const int32 TotalSamples = LocalSamplePoints.Num();
    int32 CoveredCount = 0;

    for (const FVector& LocalPt : LocalSamplePoints)
    {
        const FVector WorldPt =
            WorldTransform.TransformPosition(LocalPt);

        bool bCoveredByAny = false;

        for (const FVolumeShape& Volume : ActiveVolumes)
        {
            if (!Volume.IsValid())
                continue;

            if (IsPointInsideVolume(WorldPt, Volume))
            {
                bCoveredByAny = true;
                break; // union logic
            }
        }

        // Count coverage
        if (bCoveredByAny)
        {
            ++CoveredCount;
        }


        

        if (bDrawDebugSamplePoints) //update if it is covered or not by drawing debug sphere
        {
            const FColor DebugColor =
                bCoveredByAny ? FColor::Red : FColor::Green;

            DrawDebugPoint(
                GetWorld(),
                WorldPt,
                8.f,
                DebugColor,
                false,
                0.01f//preventing flickering(this wont be triggered at delta interval)
            );
        }
    }

    // ===== is hidden or not
    const bool bFullyHidden = (CoveredCount == TotalSamples);

    //TODO:: Maybe add a threshold alpha for allowing some percentage is considered as hidden(90% could be close enough)

    UE_LOG(LOSTrace, Verbose,
        TEXT("%s [%s] coverage=%d/%d hidden=%s"),
        *Ctx,
        *GetOwner()->GetName(),
        CoveredCount,
        TotalSamples,
        bFullyHidden ? TEXT("true") : TEXT("false"));

    return bFullyHidden;
}

void UVolumeVisibilityEvaluatorComp::BakeSamplePoints()
{
    const FString Ctx = TopDownVisionDebug::GetClientDebugName(this);

    if (!TargetShapeComp)
    {
        UE_LOG(LOSTrace, Warning,
            TEXT("%s UVisibilityTargetComp::BakeSamplePoints >> TargetShapeComp is null on [%s] — aborting"),
            *Ctx, *GetOwner()->GetName());
        return;
    }

    const int32 PrevCount = LocalSamplePoints.Num();
    LocalSamplePoints = SampleShapeLocal(TargetShapeComp);

    UE_LOG(LOSTrace, Log,
        TEXT("%s UVisibilityTargetComp::BakeSamplePoints >> [%s] baked %d points (was %d) | spacing=%.1f | shape=[%s]"),
        *Ctx,
        *GetOwner()->GetName(),
        LocalSamplePoints.Num(),
        PrevCount,
        SampleSpacing,
        *TargetShapeComp->GetClass()->GetName());
}

void UVolumeVisibilityEvaluatorComp::DebugDrawSamplePoints(float Duration) const
{
    const FString Ctx = TopDownVisionDebug::GetClientDebugName(this);

    if (LocalSamplePoints.IsEmpty())
    {
        UE_LOG(LOSTrace, Warning,
            TEXT("%s UVisibilityTargetComp::DebugDrawSamplePoints >> no baked points on [%s] — run BakeSamplePoints first"),
            *Ctx, *GetOwner()->GetName());
        return;
    }

    if (!TargetShapeComp)
    {
        UE_LOG(LOSTrace, Warning,
            TEXT("%s UVisibilityTargetComp::DebugDrawSamplePoints >> TargetShapeComp is null on [%s]"),
            *Ctx, *GetOwner()->GetName());
        return;
    }

    const UWorld* World = GetWorld();
    if (!World) return;

    const FTransform& WorldTransform = TargetShapeComp->GetComponentTransform();//get transform value for sample point offset

    for (const FVector& LocalPt : LocalSamplePoints)
    {
        const FVector WorldPt = WorldTransform.TransformPosition(LocalPt);
        DrawDebugSphere(
            World,
            WorldPt,
            2.f,
            4,
            FColor::Cyan,
            false,
            Duration);
    }

    UE_LOG(LOSTrace, Log,
        TEXT("%s UVisibilityTargetComp::DebugDrawSamplePoints >> drew %d points on [%s] | duration=%.1fs"),
        *Ctx, LocalSamplePoints.Num(), *GetOwner()->GetName(), Duration);
}

void UVolumeVisibilityEvaluatorComp::OnVolumeOverlapBegin(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{if (!OtherComp)
    return;

    FVolumeShape NewVolume;
    NewVolume.Component = OtherComp;

    if (const USphereComponent* Sphere = Cast<USphereComponent>(OtherComp))
    {
        NewVolume.ShapeType = EShapeType::Sphere;
        NewVolume.Radius = Sphere->GetUnscaledSphereRadius();
    }
    else if (const UBoxComponent* Box = Cast<UBoxComponent>(OtherComp))
    {
        NewVolume.ShapeType = EShapeType::Box;
        NewVolume.BoxExtent = Box->GetUnscaledBoxExtent();
    }
    else if (const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(OtherComp))
    {
        NewVolume.ShapeType = EShapeType::Capsule;
        NewVolume.Radius = Capsule->GetUnscaledCapsuleRadius();
        NewVolume.CapsuleCylHalf =
            Capsule->GetUnscaledCapsuleHalfHeight() - NewVolume.Radius;
    }
    else
    {
        NewVolume.ShapeType = EShapeType::Bounds;
        const FBoxSphereBounds Bounds = OtherComp->CalcLocalBounds();
        NewVolume.BoxExtent = Bounds.BoxExtent;
    }

    ActiveVolumes.Add(NewVolume);
}

void UVolumeVisibilityEvaluatorComp::OnVolumeOverlapEnd(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex)
{
    ActiveVolumes.RemoveAll(
        [OtherComp](const FVolumeShape& V)
        {
            return V.Component.Get() == OtherComp;
        });

    UE_LOG(LOSTrace, Log,
        TEXT("%s UVisibilityTargetComp::OnVolumeOverlapEnd >> [%s] left volume [%s] | active volumes=%d"),
        *TopDownVisionDebug::GetClientDebugName(this),
        *GetOwner()->GetName(),
        *OtherComp->GetName(),
        ActiveVolumes.Num());
}


TArray<FVector> UVolumeVisibilityEvaluatorComp::SampleShapeLocal(UPrimitiveComponent* ShapeComp) const
{
    const FString Ctx = TopDownVisionDebug::GetClientDebugName(this);

    if (const USphereComponent* Sphere = Cast<USphereComponent>(ShapeComp))
    {
        UE_LOG(LOSTrace, Log,
            TEXT("%s UVisibilityTargetComp::SampleShapeLocal >> dispatching to SampleSphere"),
            *Ctx);
        return SampleSphere(Sphere);
    }
    if (const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(ShapeComp))
    {
        UE_LOG(LOSTrace, Log,
            TEXT("%s UVisibilityTargetComp::SampleShapeLocal >> dispatching to SampleCapsule"),
            *Ctx);
        return SampleCapsule(Capsule);
    }
    if (const UBoxComponent* Box = Cast<UBoxComponent>(ShapeComp))
    {
        UE_LOG(LOSTrace, Log,
            TEXT("%s UVisibilityTargetComp::SampleShapeLocal >> dispatching to SampleBox"),
            *Ctx);
        return SampleBox(Box);
    }

    UE_LOG(LOSTrace, Warning,
        TEXT("%s UVisibilityTargetComp::SampleShapeLocal >> unrecognized shape type [%s] — falling back to SampleBounds"),
        *Ctx, *ShapeComp->GetClass()->GetName());

    return SampleBounds(ShapeComp);
}

//Shape sampler internal functions

TArray<FVector> UVolumeVisibilityEvaluatorComp::SampleSphere(const USphereComponent* Sphere) const
{
    TArray<FVector> Points;

    const float Radius = Sphere->GetUnscaledSphereRadius();
    const float SurfaceArea = 4.f * PI * Radius * Radius;
    const int32 NumPoints = FMath::Max(8, FMath::RoundToInt(SurfaceArea / (SampleSpacing * SampleSpacing)));

    Points.Reserve(NumPoints);

    const float GoldenAngle = PI * (3.f - FMath::Sqrt(5.f));

    for (int32 i = 0; i < NumPoints; ++i)
    {
        const float Y = 1.f - (2.f * i / FMath::Max(NumPoints - 1, 1));
        const float RadiusAtY = FMath::Sqrt(FMath::Max(0.f, 1.f - Y * Y));
        const float Theta = GoldenAngle * i;

        Points.Add(FVector(FMath::Cos(Theta) * RadiusAtY, FMath::Sin(Theta) * RadiusAtY, Y) * Radius);
    }

    UE_LOG(LOSTrace, Log,
        TEXT("%s UVisibilityTargetComp::SampleSphere >> radius=%.1f | points=%d"),
        *TopDownVisionDebug::GetClientDebugName(this), Radius, Points.Num());

    return Points;
}


TArray<FVector> UVolumeVisibilityEvaluatorComp::SampleCapsule(const UCapsuleComponent* Capsule) const
{
     TArray<FVector> Points;

    const float Radius = Capsule->GetUnscaledCapsuleRadius();
    const float HalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
    const float CylHalfHeight = HalfHeight - Radius;

    // --- Cylinder band ---
    const int32 NumRings     = FMath::Max(1, FMath::RoundToInt((CylHalfHeight * 2.f) / SampleSpacing));
    const int32 PointsPerRing = FMath::Max(4, FMath::RoundToInt((2.f * PI * Radius) / SampleSpacing));

    for (int32 Ring = 0; Ring <= NumRings; ++Ring)
    {
        const float Z = (NumRings == 0) ? 0.f : FMath::Lerp(-CylHalfHeight, CylHalfHeight, (float)Ring / NumRings);

        for (int32 j = 0; j < PointsPerRing; ++j)
        {
            const float Angle = (2.f * PI * j) / PointsPerRing;
            Points.Add(FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, Z));
        }
    }

    // --- Hemispheres via Fibonacci sphere split on Z ---
    const float HemiArea = 2.f * PI * Radius * Radius;
    const int32 HemiPoints = FMath::Max(4, FMath::RoundToInt(HemiArea / (SampleSpacing * SampleSpacing)));
    const int32 TotalSpherePoints = HemiPoints * 2;
    const float GoldenAngle = PI * (3.f - FMath::Sqrt(5.f));

    for (int32 i = 0; i < TotalSpherePoints; ++i)
    {
        const float NormY = 1.f - (2.f * i / FMath::Max(TotalSpherePoints - 1, 1));
        const float R = FMath::Sqrt(FMath::Max(0.f, 1.f - NormY * NormY));
        const float Theta = GoldenAngle * i;

        const FVector Scaled = FVector(FMath::Cos(Theta) * R, FMath::Sin(Theta) * R, NormY) * Radius;

        if (Scaled.Z >= 0.f)
            Points.Add(FVector(Scaled.X, Scaled.Y,  Scaled.Z + CylHalfHeight));
        else
            Points.Add(FVector(Scaled.X, Scaled.Y,  Scaled.Z - CylHalfHeight));
    }

    UE_LOG(LOSTrace, Log,
        TEXT("%s UVisibilityTargetComp::SampleCapsule >> radius=%.1f | halfHeight=%.1f | cylHalf=%.1f | points=%d"),
        *TopDownVisionDebug::GetClientDebugName(this), Radius, HalfHeight, CylHalfHeight, Points.Num());

    return Points;
}



TArray<FVector> UVolumeVisibilityEvaluatorComp::SampleBox(const UBoxComponent* Box) const
{
    TArray<FVector> Points;

    const FVector Extent = Box->GetUnscaledBoxExtent();

    SampleFace(
        Points,
        FVector( Extent.X, 0, 0),
        FVector(0,1,0),
        Extent.Y,
        FVector(0,0,1),
        Extent.Z);
    SampleFace(
        Points,
        FVector(-Extent.X, 0, 0),
        FVector(0,1,0),
        Extent.Y,
        FVector(0,0,1),
        Extent.Z);
    SampleFace(
        Points,
        FVector(0,  Extent.Y, 0),
        FVector(1,0,0),
        Extent.X,
        FVector(0,0,1),
        Extent.Z);
    SampleFace(
        Points,
        FVector(0, -Extent.Y, 0),
        FVector(1,0,0),
        Extent.X,
        FVector(0,0,1),
        Extent.Z);
    SampleFace(
        Points,
        FVector(0, 0,  Extent.Z),
        FVector(1,0,0),
        Extent.X,
        FVector(0,1,0),
        Extent.Y);
    SampleFace(
        Points,
        FVector(0, 0, -Extent.Z),
        FVector(1,0,0),
        Extent.X,
        FVector(0,1,0),
        Extent.Y);

    UE_LOG(LOSTrace, Log,
        TEXT("%s UVisibilityTargetComp::SampleBox >> extent=(%.1f, %.1f, %.1f) | points=%d"),
        *TopDownVisionDebug::GetClientDebugName(this), Extent.X, Extent.Y, Extent.Z, Points.Num());

    return Points;
}


TArray<FVector> UVolumeVisibilityEvaluatorComp::SampleBounds(const UPrimitiveComponent* Comp) const
{
    TArray<FVector> Points;

    const FBoxSphereBounds LocalBounds = Comp->CalcLocalBounds();
    const FVector Extent = LocalBounds.BoxExtent;
    const FVector Origin = LocalBounds.Origin;
    
    SampleFace(
        Points,
        FVector( Extent.X, 0, 0),
        FVector(0,1,0),
        Extent.Y,
        FVector(0,0,1),
        Extent.Z);
    SampleFace(
        Points,
        FVector(-Extent.X, 0, 0),
        FVector(0,1,0),
        Extent.Y,
        FVector(0,0,1),
        Extent.Z);
    SampleFace(
        Points,
        FVector(0,  Extent.Y, 0),
        FVector(1,0,0),
        Extent.X,
        FVector(0,0,1),
        Extent.Z);
    SampleFace(
        Points,
        FVector(0, -Extent.Y, 0),
        FVector(1,0,0),
        Extent.X,
        FVector(0,0,1),
        Extent.Z);
    SampleFace(
        Points,
        FVector(0, 0,  Extent.Z),
        FVector(1,0,0),
        Extent.X,
        FVector(0,1,0),
        Extent.Y);
    SampleFace(
        Points,
        FVector(0, 0, -Extent.Z),
        FVector(1,0,0),
        Extent.X,
        FVector(0,1,0),
        Extent.Y);

    UE_LOG(LOSTrace, Log,
        TEXT("%s UVisibilityTargetComp::SampleBounds >> [fallback] extent=(%.1f, %.1f, %.1f) | points=%d"),
        *TopDownVisionDebug::GetClientDebugName(this), Extent.X, Extent.Y, Extent.Z, Points.Num());

    return Points;
}

//internal function for face sampler
void UVolumeVisibilityEvaluatorComp::SampleFace(TArray<FVector>& OutPoints, const FVector& FaceOffset,
    const FVector& AxisU, float HalfU, const FVector& AxisV, float HalfV, const FVector& Origin) const
{
    const int32 StepsU = FMath::Max(1, FMath::RoundToInt((HalfU * 2.f) / SampleSpacing));
    const int32 StepsV = FMath::Max(1, FMath::RoundToInt((HalfV * 2.f) / SampleSpacing));

    for (int32 u = 0; u <= StepsU; ++u)
    {
        for (int32 v = 0; v <= StepsV; ++v)
        {
            const float U = FMath::Lerp(-HalfU, HalfU, (float)u / StepsU);
            const float V = FMath::Lerp(-HalfV, HalfV, (float)v / StepsV);
            OutPoints.Add(Origin + FaceOffset + AxisU * U + AxisV * V);
        }
    }
}

//  Volume containment helper
bool UVolumeVisibilityEvaluatorComp::IsPointInsideVolume(const FVector& WorldPoint, const FVolumeShape& Volume) const
{
    if (!Volume.Component.IsValid())
        return false;

    const FTransform& Transform =
        Volume.Component->GetComponentTransform();

    const FVector LocalPoint =
        Transform.InverseTransformPosition(WorldPoint);

    switch (Volume.ShapeType)
    {
    case EShapeType::Sphere:
        return LocalPoint.SizeSquared() <= FMath::Square(Volume.Radius);

    case EShapeType::Box:
        return FMath::Abs(LocalPoint.X) <= Volume.BoxExtent.X &&
               FMath::Abs(LocalPoint.Y) <= Volume.BoxExtent.Y &&
               FMath::Abs(LocalPoint.Z) <= Volume.BoxExtent.Z;

    case EShapeType::Capsule:
        {
            const float ClampedZ = FMath::Clamp(
                LocalPoint.Z,
                -Volume.CapsuleCylHalf,
                 Volume.CapsuleCylHalf);

            const FVector Delta =
                LocalPoint - FVector(0.f, 0.f, ClampedZ);

            return Delta.SizeSquared() <=
                   FMath::Square(Volume.Radius);
        }

    case EShapeType::Bounds:
        {
            return FMath::Abs(LocalPoint.X) <= Volume.BoxExtent.X &&
                   FMath::Abs(LocalPoint.Y) <= Volume.BoxExtent.Y &&
                   FMath::Abs(LocalPoint.Z) <= Volume.BoxExtent.Z;
        }

    default:
        return false;
    }
}

