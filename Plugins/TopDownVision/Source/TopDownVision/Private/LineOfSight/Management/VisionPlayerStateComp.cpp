#include "LineOfSight/Management/VisionPlayerStateComp.h"

#include "Net/UnrealNetwork.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "LineOfSight/Management/VisionGameStateComp.h"
#include "LineOfSight/VisionComps/Vision_VisualComp.h"
#include "LineOfSight/VisionComps/Vision_EvaluatorComp.h"
#include "LineOfSight/Management/Subsystem/LOSVisionSubsystem.h"

DEFINE_LOG_CATEGORY(VisionPlayerStateComp);

UVisionPlayerStateComp::UVisionPlayerStateComp()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UVisionPlayerStateComp::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UVisionPlayerStateComp, TeamChannel);
    DOREPLIFETIME(UVisionPlayerStateComp, bAllReveal);
}

void UVisionPlayerStateComp::BeginPlay()
{
    Super::BeginPlay();
    if (UWorld* World = GetWorld())
        World->GetTimerManager().SetTimerForNextTick(
            this, &UVisionPlayerStateComp::RefreshVisibility);
}

// -------------------------------------------------------------------------- //
//  Team
// -------------------------------------------------------------------------- //

void UVisionPlayerStateComp::SetTeamChannel(EVisionChannel InTeam)
{
    TeamChannel = InTeam;

    UE_LOG(VisionPlayerStateComp, Log,
        TEXT("[%s] SetTeamChannel >> %d"),
        *GetOwner()->GetName(), (uint8)TeamChannel);

    InitializeSameTeamEvaluators();
    RefreshVisibility();
}

void UVisionPlayerStateComp::OnRep_TeamChannel()
{
    UE_LOG(VisionPlayerStateComp, Log,
        TEXT("[%s] OnRep_TeamChannel >> %d"),
        *GetOwner()->GetName(), (uint8)TeamChannel);

    InitializeSameTeamEvaluators();
    RefreshVisibility();
}

// -------------------------------------------------------------------------- //
//  All Reveal
// -------------------------------------------------------------------------- //

void UVisionPlayerStateComp::SetAllReveal(bool bEnabled)
{
    if (!GetOwner()->HasAuthority())
    {
        UE_LOG(VisionPlayerStateComp, Warning,
            TEXT("[%s] SetAllReveal >> Server only"), *GetOwner()->GetName());
        return;
    }

    bAllReveal = bEnabled;

    UE_LOG(VisionPlayerStateComp, Log,
        TEXT("[%s] SetAllReveal >> %s"),
        *GetOwner()->GetName(), bAllReveal ? TEXT("ON") : TEXT("OFF"));

    RefreshVisibility();
}

void UVisionPlayerStateComp::OnRep_AllReveal()
{
    UE_LOG(VisionPlayerStateComp, Log,
        TEXT("[%s] OnRep_AllReveal >> %s"),
        *GetOwner()->GetName(), bAllReveal ? TEXT("ON") : TEXT("OFF"));
    RefreshVisibility();
}

// -------------------------------------------------------------------------- //
//  CanSeeTeam
// -------------------------------------------------------------------------- //

bool UVisionPlayerStateComp::CanSeeTeam(EVisionChannel InTeam) const
{
    if (InTeam == EVisionChannel::AlwaysVisible)
        return true;

    return bAllReveal || (TeamChannel == InTeam);
}

// -------------------------------------------------------------------------- //
//  Same-team evaluator initialization
// -------------------------------------------------------------------------- //

void UVisionPlayerStateComp::InitializeSameTeamEvaluators()
{
    if (TeamChannel == EVisionChannel::None)
        return;

    UWorld* World = GetWorld();
    if (!World) return;

    APlayerController* PC = GEngine->GetFirstLocalPlayerController(World);
    if (!PC) return;

    ULOSVisionSubsystem* Subsystem = World->GetSubsystem<ULOSVisionSubsystem>();
    if (!Subsystem) return;

    // Iterate ALL registered providers across every channel.
    // CanSeeTeam decides eligibility — same logic the RT manager uses
    // to decide which providers to stamp, so evaluators and RT stay in sync.
    for (UVision_VisualComp* Provider : Subsystem->GetAllProviders())
    {
        if (!Provider || !Provider->GetOwner())
            continue;

        // Skip if this player cannot see this provider's channel.
        if (!CanSeeTeam(Provider->GetVisionChannel()))
            continue;

        // Skip the locally controlled pawn — already initialized.
        APawn* Pawn = Cast<APawn>(Provider->GetOwner());
        if (Pawn && Pawn->IsLocallyControlled())
            continue;

        UVision_EvaluatorComp* Evaluator =
            Provider->GetOwner()->FindComponentByClass<UVision_EvaluatorComp>();
        if (!Evaluator)
            continue;

        Evaluator->InitializeIfSameTeam();
    }

    UE_LOG(VisionPlayerStateComp, Log,
        TEXT("[%s] InitializeSameTeamEvaluators >> Done for team %d"),
        *GetOwner()->GetName(), (uint8)TeamChannel);
}

// -------------------------------------------------------------------------- //
//  ReevaluateTargetVisibility
// -------------------------------------------------------------------------- //

void UVisionPlayerStateComp::ReevaluateTargetVisibility(
    AActor* Target, EVisionChannel ExcludeObserverTeam)
{
    if (!Target) return;

    UVision_VisualComp* VisualComp =
        Target->FindComponentByClass<UVision_VisualComp>();
    if (!VisualComp) return;

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC || !PC->IsLocalController()) return;

    bool bShouldBeVisible = false;

    if (bAllReveal)
    {
        bShouldBeVisible = true;
    }
    else
    {
        const EVisionChannel TargetTeam = VisualComp->GetVisionChannel();
        if (CanSeeTeam(TargetTeam))
        {
            bShouldBeVisible = true;
        }
        else
        {
            // --- Pass 1: local vote map ---
            // Updated synchronously before any RPC — zero latency for the
            // evaluating client.
            ULOSVisionSubsystem* Subsystem =
                GetWorld()->GetSubsystem<ULOSVisionSubsystem>();
            if (Subsystem)
            {
                const TMap<uint8, TSet<TWeakObjectPtr<AActor>>>* VoteMap =
                    Subsystem->GetVisibilityVotesForTarget(Target);

                if (VoteMap)
                {
                    for (const TPair<uint8, TSet<TWeakObjectPtr<AActor>>>& TeamPair
                        : *VoteMap)
                    {
                        EVisionChannel EntryTeam = (EVisionChannel)TeamPair.Key;

                        if (ExcludeObserverTeam != EVisionChannel::None &&
                            EntryTeam == ExcludeObserverTeam)
                            continue;

                        if (TeamPair.Value.Num() == 0)
                            continue;

                        if (CanSeeTeam(EntryTeam))
                        {
                            bShouldBeVisible = true;
                            break;
                        }
                    }
                }
            }

            // --- Pass 2: GSComp replicated state ---
            // Catches shared vision from teammates on other machines.
            // Player B has no local vote for Player A's sighting — this
            // pass finds it via FastArray replication.
            if (!bShouldBeVisible)
            {
                UVisionGameStateComp* GSComp = nullptr;
                if (AGameStateBase* GS = GetWorld()->GetGameState())
                    GSComp = GS->FindComponentByClass<UVisionGameStateComp>();

                if (GSComp)
                {
                    for (const FVisibleActorEntry& Entry : GSComp->GetVisibleActors())
                    {
                        if (Entry.Target != Target)
                            continue;

                        if (ExcludeObserverTeam != EVisionChannel::None &&
                            Entry.ObserverTeam == ExcludeObserverTeam)
                            continue;

                        if (CanSeeTeam(Entry.ObserverTeam))
                        {
                            bShouldBeVisible = true;
                            break;
                        }
                    }
                }
            }
        }
    }

    VisualComp->SetVisible(bShouldBeVisible);
}

// -------------------------------------------------------------------------- //
//  RefreshVisibility
// -------------------------------------------------------------------------- //

void UVisionPlayerStateComp::RefreshVisibility()
{
    UWorld* World = GetWorld();
    if (!World) return;

    AGameStateBase* GS = World->GetGameState();
    if (!GS) return;

    UVisionGameStateComp* GSComp =
        GS->FindComponentByClass<UVisionGameStateComp>();
    if (!GSComp) return;

    GSComp->FlushPendingReveals(this);

    TSet<AActor*> Evaluated;
    for (const FVisibleActorEntry& Entry : GSComp->GetVisibleActors())
    {
        if (!Entry.Target || Evaluated.Contains(Entry.Target))
            continue;

        Evaluated.Add(Entry.Target);
        ReevaluateTargetVisibility(Entry.Target);
    }

    UE_LOG(VisionPlayerStateComp, Verbose,
        TEXT("[%s] RefreshVisibility >> %d unique actors | Team:%d | AllReveal:%d"),
        *GetOwner()->GetName(),
        Evaluated.Num(),
        (uint8)TeamChannel,
        (int32)bAllReveal);
}