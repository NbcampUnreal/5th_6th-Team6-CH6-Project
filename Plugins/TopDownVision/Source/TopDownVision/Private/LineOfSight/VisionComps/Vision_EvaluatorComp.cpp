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
    SetIsReplicatedByDefault(true);

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
    // Initialization is driven externally by the owner via InitializeEvaluator()
}

// -------------------------------------------------------------------------- //
//  Initialization
// -------------------------------------------------------------------------- //

void UVision_EvaluatorComp::PrepareDetectionSphere()
{
    if (!DetectionSphere)
        return;

    if (USceneComponent* Root = GetOwner()->GetRootComponent())
    {
        DetectionSphere->AttachToComponent(
            Root, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    }

    if (bDrawDebug)
    {
        DetectionSphere->SetVisibility(true);
        DetectionSphere->SetHiddenInGame(false);
    }

    DetectionSphere->OnComponentBeginOverlap.AddDynamic(
        this, &UVision_EvaluatorComp::OnDetectionSphereBeginOverlap);
    DetectionSphere->OnComponentEndOverlap.AddDynamic(
        this, &UVision_EvaluatorComp::OnDetectionSphereEndOverlap);

    SyncDetectionRadius();

    // Manual check for actors already inside at init time
    TArray<AActor*> AlreadyOverlapping;
    DetectionSphere->GetOverlappingActors(AlreadyOverlapping);
    for (AActor* Actor : AlreadyOverlapping)
    {
        if (!Actor || Actor == GetOwner())
            continue;

        TArray<UPrimitiveComponent*> PrimComps;
        Actor->GetComponents<UPrimitiveComponent>(PrimComps);
        for (UPrimitiveComponent* Comp : PrimComps)
        {
            if (Comp && Comp->ComponentHasTag(TargetTag))
            {
                OverlappingTargets.Add(Actor);
                UE_LOG(LOSVision, Verbose,
                    TEXT("[%s] PrepareDetectionSphere >> Pre-existing target: %s"),
                    *TopDownVisionDebug::GetClientDebugName(GetOwner()),
                    *Actor->GetName());
                break;
            }
        }
    }

    if (!OverlappingTargets.IsEmpty())
        StartEvaluationTimer();
}

void UVision_EvaluatorComp::InitializeEvaluator(UVision_VisualComp* DirectParamComp)
{
    // Only run for the locally controlled pawn
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn || !OwnerPawn->IsLocallyControlled())
    {
        UE_LOG(LOSVision, Verbose,
            TEXT("[%s] InitializeEvaluator >> Skipped — not locally controlled"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        return;
    }

    //if (ShouldRunServerLogic())
    //    return;// no server

    if (DirectParamComp)
        DirectCacheVisualComp(DirectParamComp);
    else
        FindAndCacheVisualComp();

    PrepareDetectionSphere();
}

void UVision_EvaluatorComp::DirectCacheVisualComp(UVision_VisualComp* DirectParamComp)
{
    if (!DirectParamComp)
    {
        UE_LOG(LOSVision, Error,
            TEXT("[%s] DirectCacheVisualComp >> Null VisualComp passed"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        return;
    }

    CachedVisualComp = DirectParamComp;
    UE_LOG(LOSVision, Verbose,
        TEXT("[%s] DirectCacheVisualComp >> Cached VisualComp directly"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()));
}

void UVision_EvaluatorComp::FindAndCacheVisualComp()
{
    UVision_VisualComp* VisualComp = GetOwner()->FindComponentByClass<UVision_VisualComp>();
    if (!VisualComp)
    {
        UE_LOG(LOSVision, Error,
            TEXT("[%s] FindAndCacheVisualComp >> No Vision_VisualComp found on owner"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        return;
    }

    CachedVisualComp = VisualComp;
    UE_LOG(LOSVision, Verbose,
        TEXT("[%s] FindAndCacheVisualComp >> VisualComp cached successfully"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()));
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

    DetectionRadius = CachedVisualComp->GetVisibleRange();
    DetectionSphere->SetSphereRadius(DetectionRadius);

    UE_LOG(LOSVision, Verbose,
        TEXT("[%s] SyncDetectionRadius >> Radius set to %.1f"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()),
        DetectionRadius);
}

void UVision_EvaluatorComp::BP_DrawDebugSphereComp(float DrawTime)
{
    if (bDrawDebug)
    {
        DrawDebugSphere(
            GetWorld(),
            GetOwner()->GetActorLocation(),
            DetectionRadius,
            32,
            FColor::Yellow,
            false,
            DrawTime);
    }
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

    UE_LOG(LOSVision, Verbose,
        TEXT("[%s] OnBeginOverlap >> Target entered: %s"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()),
        *OtherActor->GetName());

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
    LastReportedVisibility.Remove(OtherActor);

    UE_LOG(LOSVision, Verbose,
        TEXT("[%s] OnEndOverlap >> Target left: %s"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()),
        *OtherActor->GetName());

    // Always report false — vote system keeps visible if another observer still sees it
    // PlayerStateComp handles same-team always-visible on the display side
    ReportVisibility(OtherActor, false);

    if (OverlappingTargets.IsEmpty())
        StopEvaluationTimer();
}

// -------------------------------------------------------------------------- //
//  Evaluation tick
// -------------------------------------------------------------------------- //

void UVision_EvaluatorComp::EvaluateTick()
{
    if (OverlappingTargets.IsEmpty())
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

// -------------------------------------------------------------------------- //
//  Visibility reporting
// -------------------------------------------------------------------------- //

void UVision_EvaluatorComp::ReportVisibilityIfChanged(AActor* Target, bool bVisible)
{
    // No same-team skip here — vote system handles multi-observer correctly
    // PlayerStateComp handles same-team always-visible on the display side
    bool* LastState = LastReportedVisibility.Find(Target);
    if (LastState && *LastState == bVisible)
        return;

    LastReportedVisibility.FindOrAdd(Target) = bVisible;
    ReportVisibility(Target, bVisible);
}

void UVision_EvaluatorComp::ReportVisibility(AActor* Target, bool bVisible)
{
    if (!CachedVisualComp || !Target)
        return;

    UE_LOG(LOSVision, Warning,
        TEXT("[%s] ReportVisibility >> Target:%s | Channel:%s | Visible:%d"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()),
        *Target->GetName(),
        *UEnum::GetValueAsString(CachedVisualComp->GetVisionChannel()),
        bVisible);

    if (GetWorld()->GetNetMode() == NM_Standalone)
    {
        ULOSVisionSubsystem* Subsystem = GetWorld()->GetSubsystem<ULOSVisionSubsystem>();
        if (!Subsystem)
        {
            UE_LOG(LOSVision, Warning,
                TEXT("[%s] ReportVisibility >> LOSVisionSubsystem not found"),
                *TopDownVisionDebug::GetClientDebugName(GetOwner()));
            return;
        }
        Subsystem->ReportTargetVisibility(
            GetOwner(), CachedVisualComp->GetVisionChannel(), Target, bVisible);
    }
    else
    {
        Server_ReportVisibility(Target, CachedVisualComp->GetVisionChannel(), bVisible);
    }
}

void UVision_EvaluatorComp::Server_ReportVisibility_Implementation(
    AActor* Target, EVisionChannel Channel, bool bVisible)
{
    ULOSVisionSubsystem* Subsystem = GetWorld()->GetSubsystem<ULOSVisionSubsystem>();
    if (!Subsystem)
    {
        UE_LOG(LOSVision, Warning,
            TEXT("[%s] Server_ReportVisibility >> LOSVisionSubsystem not found"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        return;
    }

    Subsystem->ReportTargetVisibility(GetOwner(), Channel, Target, bVisible);
}

// -------------------------------------------------------------------------- //
//  Evaluate
// -------------------------------------------------------------------------- //

void UVision_EvaluatorComp::EvaluateTarget(AActor* Target, UVision_VisualComp* TargetVisual)
{
    if (!Target || !TargetVisual || !CachedVisualComp)
        return;

    UTopDown2DShapeComp* ShapeComp = TargetVisual->GetShapeComp();
    if (!ShapeComp)
    {
        UE_LOG(LOSVision, Warning,
            TEXT("[%s] EvaluateTarget >> No ShapeComp on target: %s"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()),
            *Target->GetName());
        return;
    }

    const bool bVolumeVisible = EvaluateVolumeObstacle(Target, ShapeComp);
    if (!bVolumeVisible)
    {
        UE_LOG(LOSVision, Verbose,
            TEXT("[%s] EvaluateTarget >> %s hidden by volume"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()),
            *Target->GetName());

        ReportVisibilityIfChanged(Target, false);
        return;
    }

    const bool bWallVisible = EvaluateWallObstacle(Target, ShapeComp);

    UE_LOG(LOSVision, Verbose,
        TEXT("[%s] EvaluateTarget >> %s | Volume: VISIBLE | Wall: %s"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()),
        *Target->GetName(),
        bWallVisible ? TEXT("VISIBLE") : TEXT("HIDDEN"));

    ReportVisibilityIfChanged(Target, bWallVisible);
}

bool UVision_EvaluatorComp::EvaluateWallObstacle(AActor* Target, UTopDown2DShapeComp* ShapeComp)
{
    // temp — always visible
   // return true;

    return UWallVisibilityEvaluator2D::EvaluateVisibility(
        GetWorld(),
        GetOwner()->GetActorLocation(),
        Target->GetActorLocation(),
        ShapeComp,
        WallTraceChannel);
}

bool UVision_EvaluatorComp::EvaluateVolumeObstacle(AActor* Target, UTopDown2DShapeComp* ShapeComp)
{
    // temp — always visible
    //return true;
    
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
    return GetOwner() && GetOwner()->HasAuthority();
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
        TEXT("[%s] StartEvaluationTimer >> Started"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()));
}

void UVision_EvaluatorComp::StopEvaluationTimer()
{
    GetWorld()->GetTimerManager().ClearTimer(EvaluationTimerHandle);

    UE_LOG(LOSVision, Verbose,
        TEXT("[%s] StopEvaluationTimer >> Stopped"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()));
}