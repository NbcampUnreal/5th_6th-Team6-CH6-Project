#include "LineOfSight/VisionComps/Vision_EvaluatorComp.h"

#include "Components/SphereComponent.h"
#include "LineOfSight/VisionComps/Vision_VisualComp.h"
#include "LineOfSight/ObjectTracing/TopDown2DShapeComp.h"
#include "TopDownVisionDebug.h"
#include "GameFramework/PlayerState.h"
#include "LineOfSight/Management/VisionPlayerStateComp.h"
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
    // Initialization is driven externally via InitializeEvaluator()
    // or late via InitializeIfSameTeam() when team replicates.
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
    if (bIsInitialized)
        return;

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    const bool bIsLocallyControlled = OwnerPawn && OwnerPawn->IsLocallyControlled();
    const bool bIsSameTeam = IsSameTeamAsLocalPlayer();

    if (!bIsLocallyControlled && !bIsSameTeam)
    {
        UE_LOG(LOSVision, Verbose,
            TEXT("[%s] InitializeEvaluator >> Skipped — not local and not same team"),
            *TopDownVisionDebug::GetClientDebugName(GetOwner()));
        return;
    }

    if (DirectParamComp)
        DirectCacheVisualComp(DirectParamComp);
    else
        FindAndCacheVisualComp();

    PrepareDetectionSphere();
    bIsInitialized = true;

    UE_LOG(LOSVision, Verbose,
        TEXT("[%s] InitializeEvaluator >> Initialized (%s)"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()),
        bIsLocallyControlled ? TEXT("local") : TEXT("same team"));
}

void UVision_EvaluatorComp::InitializeIfSameTeam()
{
    if (bIsInitialized)
        return;

    if (!IsSameTeamAsLocalPlayer())
        return;

    FindAndCacheVisualComp();
    PrepareDetectionSphere();
    bIsInitialized = true;

    UE_LOG(LOSVision, Log,
        TEXT("[%s] InitializeIfSameTeam >> Late-initialized as same-team evaluator"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()));
}

bool UVision_EvaluatorComp::IsSameTeamAsLocalPlayer() const
{
    UVisionPlayerStateComp* LocalVPS =
        ULOSVisionSubsystem::GetLocalVisionPS(GetWorld());

    if (!LocalVPS || LocalVPS->GetTeamChannel() == EVisionChannel::None)
        return false;

    UVision_VisualComp* VC = CachedVisualComp
        ? CachedVisualComp
        : GetOwner()->FindComponentByClass<UVision_VisualComp>();

    if (!VC)
        return false;

    // Use CanSeeTeam instead of strict equality so AlwaysVisible and any
    // other eligible channels are included, matching what the RT manager
    // uses to decide which providers to stamp.
    return LocalVPS->CanSeeTeam(VC->GetVisionChannel());
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
    UVision_VisualComp* VisualComp =
        GetOwner()->FindComponentByClass<UVision_VisualComp>();
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
    if (bEnabled) StartEvaluationTimer();
    else          StopEvaluationTimer();
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
            DetectionRadius, 32,
            FColor::Yellow, false, DrawTime);
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

    // Cancel any pending hide — we will force-hide immediately below.
    if (FTimerHandle* Handle = PendingHideTimers.Find(OtherActor))
    {
        GetWorld()->GetTimerManager().ClearTimer(*Handle);
        PendingHideTimers.Remove(OtherActor);
    }

    LastReportedVisibility.Remove(OtherActor);

    UE_LOG(LOSVision, Verbose,
        TEXT("[%s] OnEndOverlap >> Target left: %s"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()),
        *OtherActor->GetName());

    // Leaving the sphere is authoritative — hide immediately, no hysteresis.
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
        if (!Target) continue;

        UVision_VisualComp* TargetVisual = GetVisualComp(Target);
        if (!TargetVisual) continue;

        EvaluateTarget(Target, TargetVisual);
    }
}

// -------------------------------------------------------------------------- //
//  Visibility reporting
// -------------------------------------------------------------------------- //

void UVision_EvaluatorComp::ReportVisibilityIfChanged(AActor* Target, bool bVisible)
{
    bool* LastState = LastReportedVisibility.Find(Target);
    if (LastState && *LastState == bVisible)
        return;

    if (bVisible)
    {
        // Cancel any pending hide — target is visible again.
        if (FTimerHandle* Handle = PendingHideTimers.Find(Target))
        {
            GetWorld()->GetTimerManager().ClearTimer(*Handle);
            PendingHideTimers.Remove(Target);
        }

        // Reveal immediately — no delay.
        LastReportedVisibility.FindOrAdd(Target) = true;
        ReportVisibility(Target, true);
    }
    else
    {
        // Already pending hide — do nothing.
        if (PendingHideTimers.Contains(Target))
            return;

        FTimerHandle Handle;
        GetWorld()->GetTimerManager().SetTimer(
            Handle,
            FTimerDelegate::CreateUObject(
                this, &UVision_EvaluatorComp::CommitHide, Target),
            HideHysteresisDelay,
            false);

        PendingHideTimers.Add(Target, Handle);
    }
}

void UVision_EvaluatorComp::CommitHide(AActor* Target)
{
    PendingHideTimers.Remove(Target);

    // If already committed as hidden, nothing to do.
    bool* LastState = LastReportedVisibility.Find(Target);
    if (LastState && *LastState == false)
        return;

    LastReportedVisibility.FindOrAdd(Target) = false;
    ReportVisibility(Target, false);
}

void UVision_EvaluatorComp::ReportVisibility(AActor* Target, bool bVisible)
{
    if (!CachedVisualComp || !Target)
        return;

    UE_LOG(LOSVision, Verbose,
        TEXT("[%s] ReportVisibility >> Target:%s | Channel:%s | Visible:%d"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()),
        *Target->GetName(),
        *UEnum::GetValueAsString(CachedVisualComp->GetVisionChannel()),
        bVisible);

    if (GetWorld()->GetNetMode() == NM_Standalone)
    {
        ULOSVisionSubsystem* Subsystem =
            GetWorld()->GetSubsystem<ULOSVisionSubsystem>();
        if (!Subsystem) return;
        Subsystem->ReportTargetVisibility(
            GetOwner(), CachedVisualComp->GetVisionChannel(), Target, bVisible);
        return;
    }

    // --- Multiplayer path ---

    EVisionChannel ObserverTeam = EVisionChannel::None;
    if (APawn* Pawn = Cast<APawn>(GetOwner()))
        if (APlayerState* PS = Pawn->GetPlayerState())
            if (UVisionPlayerStateComp* VPS =
                PS->FindComponentByClass<UVisionPlayerStateComp>())
                ObserverTeam = VPS->GetTeamChannel();

    // For non-player same-team actors (e.g. teammate AI / remote pawn),
    // fall back to the VisualComp's registered channel.
    if (ObserverTeam == EVisionChannel::None)
        ObserverTeam = CachedVisualComp->GetVisionChannel();

    if (ObserverTeam == EVisionChannel::None)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[Evaluator] Invalid ObserverTeam for %s"),
            *GetOwner()->GetName());
        return;
    }

    // Apply on this client immediately — do not wait for the round trip.
    if (ULOSVisionSubsystem* Subsystem =
        GetWorld()->GetSubsystem<ULOSVisionSubsystem>())
        Subsystem->ReportTargetVisibility(
            GetOwner(), ObserverTeam, Target, bVisible);

    // Inform the server so it replicates to ALL other clients.
    Server_ReportVisibility(Target, ObserverTeam, bVisible);
}

void UVision_EvaluatorComp::Server_ReportVisibility_Implementation(
    AActor* Target, EVisionChannel Channel, bool bVisible)
{
    ULOSVisionSubsystem* Subsystem =
        GetWorld()->GetSubsystem<ULOSVisionSubsystem>();
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

void UVision_EvaluatorComp::EvaluateTarget(
    AActor* Target, UVision_VisualComp* TargetVisual)
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

    const bool bWallVisible = EvaluateWallObstacle(Target, ShapeComp);

    UE_LOG(LOSVision, Verbose,
        TEXT("[%s] EvaluateTarget >> %s | Wall: %s"),
        *TopDownVisionDebug::GetClientDebugName(GetOwner()),
        *Target->GetName(),
        bWallVisible ? TEXT("VISIBLE") : TEXT("HIDDEN"));

    ReportVisibilityIfChanged(Target, bWallVisible);
}

bool UVision_EvaluatorComp::EvaluateWallObstacle(
    AActor* Target, UTopDown2DShapeComp* ShapeComp)
{
    return UWallVisibilityEvaluator2D::EvaluateVisibility(
        GetWorld(),
        GetOwner()->GetActorLocation(),
        Target->GetActorLocation(),
        ShapeComp,
        WallTraceChannel);
}

bool UVision_EvaluatorComp::EvaluateVolumeObstacle(
    AActor* Target, UTopDown2DShapeComp* ShapeComp)
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
    if (!Target) return nullptr;
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