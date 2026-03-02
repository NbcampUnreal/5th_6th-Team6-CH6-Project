// Fill out your copyright notice in the Description page of Project Settings.

#include "LineOfSight/VisionComps/Vision_EvaluatorComp.h"

#include "Components/SphereComponent.h"
#include "LineOfSight/VisionComps/Vision_VisualComp.h"
#include "LineOfSight/ObjectTracing/TopDown2DShapeComp.h"
#include "TopDownVisionDebug.h"
#include "LineOfSight/Management/Subsystem/LOSVisionSubsystem.h"
#include "LineOfSight/ObjectTracing/VolumeVisibilityEvaluator2D.h"
#include "LineOfSight/ObjectTracing/WallVisibilityEvaluator2D.h"
#include "LineOfSight/WorldObstacle/LOSObstacleDrawerComponent.h"


UVision_EvaluatorComp::UVision_EvaluatorComp()
{
    PrimaryComponentTick.bCanEverTick = false;

    //Overlap detector comp
    DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
    DetectionSphere->SetSphereRadius(DetectionRadius);
    DetectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    DetectionSphere->SetCollisionObjectType(ECC_WorldDynamic);
    DetectionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    DetectionSphere->SetCollisionResponseToChannel(VisionTargetChannel, ECR_Overlap);
    DetectionSphere->SetGenerateOverlapEvents(true);
}

void UVision_EvaluatorComp::BeginPlay()
{
    Super::BeginPlay();

    if (!ShouldRunServerLogic())
        return;

    // Resolve sibling Vision_VisualComp
    CachedVisualComp = GetOwner()->FindComponentByClass<UVision_VisualComp>();
    if (!CachedVisualComp)
    {
        UE_LOG(LOSVision, Warning,
            TEXT("[%s] UVision_EvaluatorComp::BeginPlay >> No Vision_VisualComp found on owner"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
    }

    if (DetectionSphere)
    {
        if (USceneComponent* Root = GetOwner()->GetRootComponent())
        {
            DetectionSphere->AttachToComponent(
                Root, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        }

        SyncDetectionRadius();

        DetectionSphere->OnComponentBeginOverlap.AddDynamic(
            this, &UVision_EvaluatorComp::OnDetectionSphereBeginOverlap);
        DetectionSphere->OnComponentEndOverlap.AddDynamic(
            this, &UVision_EvaluatorComp::OnDetectionSphereEndOverlap);
    }
}

// -------------------------------------------------------------------------- //
//  External control
// -------------------------------------------------------------------------- //

void UVision_EvaluatorComp::SetEvaluationEnabled(bool bEnabled)
{
    if (bEnabled)
        StartEvaluationTimer();
    else
        StopEvaluationTimer();
}

void UVision_EvaluatorComp::SyncDetectionRadius()
{
    if (!DetectionSphere || !CachedVisualComp)
        return;

    DetectionSphere->SetSphereRadius(CachedVisualComp->GetVisibleRange());

    UE_LOG(LOSVision, Verbose,
        TEXT("[%s] UVision_EvaluatorComp::SyncDetectionRadius >> Radius set to %.1f"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()),
        CachedVisualComp->GetVisibleRange());
}

// -------------------------------------------------------------------------- //
//  Overlap
// -------------------------------------------------------------------------- //

void UVision_EvaluatorComp::OnDetectionSphereBeginOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == GetOwner() || !OtherComp)
        return;

    if (!OtherComp->ComponentHasTag(TargetTag))
        return;

    OverlappingTargets.Add(OtherActor);

    UE_LOG(LOSVision, Log,
        TEXT("[%s] UVision_EvaluatorComp::OnBeginOverlap >> Target entered: %s"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()),
        *OtherActor->GetName());

    // Start evaluation timer on first target
    if (OverlappingTargets.Num() == 1)
        StartEvaluationTimer();
}

void UVision_EvaluatorComp::OnDetectionSphereEndOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex)
{
    if (!OtherActor || !OtherComp)
        return;

    if (!OtherComp->ComponentHasTag(TargetTag))
        return;

    OverlappingTargets.Remove(OtherActor);

    UE_LOG(LOSVision, Log,
        TEXT("[%s] UVision_EvaluatorComp::OnEndOverlap >> Target left: %s"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()),
        *OtherActor->GetName());

    // Stop timer when no targets remain
    if (OverlappingTargets.IsEmpty())
        StopEvaluationTimer();
}

// -------------------------------------------------------------------------- //
//  Evaluation tick
// -------------------------------------------------------------------------- //

void UVision_EvaluatorComp::EvaluateTick()
{
    if (OverlappingTargets.IsEmpty())// nothing to evaluate-> stop timer
    {
        StopEvaluationTimer();
        return;
    }

    for (AActor* Target : OverlappingTargets)
    {
        if (!Target)
            continue;

        UVision_VisualComp* TargetVisual = GetVisualComp(Target);
        if (!TargetVisual)
            continue;

        EvaluateTarget(Target, TargetVisual);
    }
}

void UVision_EvaluatorComp::ReportVisibility(AActor* Target, bool bVisible)
{
    ULOSVisionSubsystem* Subsystem = GetWorld()->GetSubsystem<ULOSVisionSubsystem>();
    if (!Subsystem)
    {
        UE_LOG(LOSVision, Warning,
            TEXT("[%s] UVision_EvaluatorComp::ReportVisibility >> LOSVisionSubsystem not found"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        return;
    }

    Subsystem->ReportTargetVisibility(
        GetOwner(),
        CachedVisualComp->GetVisionChannel(),
        Target,
        bVisible);
}

void UVision_EvaluatorComp::EvaluateTarget(AActor* Target, UVision_VisualComp* TargetVisual)
{
    if (!Target || !TargetVisual || !CachedVisualComp)
        return;

    UTopDown2DShapeComp* ShapeComp = TargetVisual->GetShapeComp();
    if (!ShapeComp)
    {
        UE_LOG(LOSVision, Warning,
            TEXT("[%s] UVision_EvaluatorComp::EvaluateTarget >> No ShapeComp on target: %s"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()),
            *Target->GetName());
        return;
    }

    // Volume first — cheap pixel sample, fast reject
    const bool bVolumeVisible = EvaluateVolumeObstacle(Target, ShapeComp);
    if (!bVolumeVisible)
    {
        UE_LOG(LOSVision, Verbose,
            TEXT("[%s] EvaluateTarget >> %s hidden by volume — skipping wall check"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()),
            *Target->GetName());

        ReportVisibility(Target, false);
        return;
    }

    // Volume says visible — now check wall
    const bool bWallVisible = EvaluateWallObstacle(Target, ShapeComp);

    UE_LOG(LOSVision, Verbose,
        TEXT("[%s] EvaluateTarget >> %s | Volume: VISIBLE | Wall: %s"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()),
        *Target->GetName(),
        bWallVisible ? TEXT("VISIBLE") : TEXT("HIDDEN"));

    ReportVisibility(Target, bWallVisible);
}

bool UVision_EvaluatorComp::EvaluateWallObstacle(AActor* Target, UTopDown2DShapeComp* ShapeComp)
{
    return UWallVisibilityEvaluator2D::EvaluateVisibility(
        GetWorld(),
        GetOwner()->GetActorLocation(),
        Target->GetActorLocation(),
        ShapeComp,
        WallTraceChannel);
}

bool UVision_EvaluatorComp::EvaluateVolumeObstacle(AActor* Target, UTopDown2DShapeComp* ShapeComp)
{
    return UVolumeVisibilityEvaluator2D::EvaluateVisibility(
         CachedVisualComp->GetObstacleDrawer()->GetObstacleRenderTarget(),
         CachedVisualComp->GetMaxVisibleRange(),
         GetOwner()->GetActorLocation(),
         Target->GetActorLocation(),
         ShapeComp,
         OcclusionThreshold);
}

// -------------------------------------------------------------------------- //
//  Helpers
// -------------------------------------------------------------------------- //

UVision_VisualComp* UVision_EvaluatorComp::GetVisualComp(AActor* Target) const
{
    if (!Target)
        return nullptr;

    return Target->FindComponentByClass<UVision_VisualComp>();
}

bool UVision_EvaluatorComp::ShouldRunServerLogic() const
{
    return GetNetMode() != NM_DedicatedServer;
}

void UVision_EvaluatorComp::StartEvaluationTimer()
{
    if (GetWorld()->GetTimerManager().IsTimerActive(EvaluationTimerHandle))
        return;

    GetWorld()->GetTimerManager().SetTimer(
        EvaluationTimerHandle,
        this,
        &UVision_EvaluatorComp::EvaluateTick,
        EvaluationInterval,
        true);

    UE_LOG(LOSVision, Verbose,
        TEXT("[%s] UVision_EvaluatorComp::StartEvaluationTimer >> Started"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()));
}

void UVision_EvaluatorComp::StopEvaluationTimer()
{
    GetWorld()->GetTimerManager().ClearTimer(EvaluationTimerHandle);

    UE_LOG(LOSVision, Verbose,
        TEXT("[%s] UVision_EvaluatorComp::StopEvaluationTimer >> Stopped"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()));
}