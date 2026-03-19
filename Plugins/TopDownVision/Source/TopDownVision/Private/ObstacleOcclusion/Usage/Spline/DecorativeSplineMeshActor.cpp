#include "ObstacleOcclusion/Usage/Spline/DecorativeSplineMeshActor.h"
#include "Components/SplineComponent.h"

ADecorativeSplineMeshActor::ADecorativeSplineMeshActor()
{
    PrimaryActorTick.bCanEverTick = false;

    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

    SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
    SplineComponent->SetupAttachment(RootComponent);
}

void ADecorativeSplineMeshActor::BeginPlay()
{
    Super::BeginPlay();
}

// ── API ───────────────────────────────────────────────────────────────────────

void ADecorativeSplineMeshActor::BuildSplineMesh()
{
    if (!IsValid(SplineComponent) || !IsValid(SplineMesh))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ADecorativeSplineMeshActor::BuildSplineMesh>> SplineComponent or SplineMesh null on %s"),
            *GetName());
        return;
    }

    ClearSplineMeshComponents();

    if (bShouldAllowScaling)
        SectionLength = GetActorScale3D().X * 100.f;

    const float SplineLength  = SplineComponent->GetSplineLength();
    const int32 TotalSegments = FMath::TruncToInt(SplineLength / SectionLength);
    const int32 LoopCount     = bShouldFullLength
                                ? FMath::Max(TotalSegments - 1, 0)
                                : TotalSegments;

    if (LoopCount <= 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ADecorativeSplineMeshActor::BuildSplineMesh>> No segments to generate on %s"),
            *GetName());
        return;
    }

    for (int32 i = 0; i < LoopCount; ++i)
        GenerateSegment(i);

    const bool DebugChecker=MarkPackageDirty();

    UE_LOG(LogTemp, Log,
        TEXT("ADecorativeSplineMeshActor::BuildSplineMesh>> Generated %d segments on %s"),
        SplineMeshComponents.Num(), *GetName());
}

// ── Internal ──────────────────────────────────────────────────────────────────

void ADecorativeSplineMeshActor::ClearSplineMeshComponents()
{
    Modify();

    for (TObjectPtr<USplineMeshComponent> Comp : SplineMeshComponents)
    {
        if (Comp)
            Comp->DestroyComponent();
    }

    SplineMeshComponents.Empty();

    const bool DebugChecker=MarkPackageDirty();
}

void ADecorativeSplineMeshActor::GenerateSegment(int32 SegmentIndex)
{
    Modify();

    USplineMeshComponent* Segment = NewObject<USplineMeshComponent>(
        this,
        USplineMeshComponent::StaticClass(),
        NAME_None,
        RF_NoFlags);

    if (!Segment) return;

    Segment->SetStaticMesh(SplineMesh);
    Segment->SetForwardAxis(ForwardAxis, false);
    Segment->SetStartScale(FVector2D(ScaleRatio, ScaleRatio), false);
    Segment->SetEndScale(FVector2D(ScaleRatio, ScaleRatio), false);

    FVector StartLocation, StartTangent, EndLocation, EndTangent;
    GetLocationAndTangent(SegmentIndex, true,  StartLocation, StartTangent);
    GetLocationAndTangent(SegmentIndex, false, EndLocation,   EndTangent);

    Segment->SetStartAndEnd(StartLocation, StartTangent, EndLocation, EndTangent, false);

    Segment->AttachToComponent(
        SplineComponent,
        FAttachmentTransformRules::KeepRelativeTransform);

    Segment->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    if (bApplyOcclusionTag && !OcclusionMeshTag.IsNone())
        Segment->ComponentTags.AddUnique(OcclusionMeshTag);

    Segment->RegisterComponent();
    AddInstanceComponent(Segment);
    Segment->UpdateMesh();

    SplineMeshComponents.Add(Segment);
}

void ADecorativeSplineMeshActor::GetLocationAndTangent(
    int32 SegmentIndex,
    bool  bIsStart,
    FVector& OutLocation,
    FVector& OutTangent) const
{
    const float Distance = (bIsStart ? SegmentIndex : SegmentIndex + 1) * SectionLength;

    OutLocation = SplineComponent->GetLocationAtDistanceAlongSpline(
        Distance, ESplineCoordinateSpace::Local);

    OutTangent = SplineComponent->GetTangentAtDistanceAlongSpline(
        Distance, ESplineCoordinateSpace::Local);

    OutTangent = OutTangent.GetSafeNormal() * SectionLength;
}