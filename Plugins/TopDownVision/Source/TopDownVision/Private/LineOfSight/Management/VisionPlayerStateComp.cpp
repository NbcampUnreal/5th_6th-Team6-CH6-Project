#include "LineOfSight/Management/VisionPlayerStateComp.h"

#include "Net/UnrealNetwork.h"
#include "GameFramework/GameStateBase.h"
#include "LineOfSight/Management/VisionGameStateComp.h"
#include "LineOfSight/VisionComps/Vision_VisualComp.h"

DEFINE_LOG_CATEGORY(VisionPlayerStateComp);

UVisionPlayerStateComp::UVisionPlayerStateComp()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UVisionPlayerStateComp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UVisionPlayerStateComp, TeamChannel);
    DOREPLIFETIME(UVisionPlayerStateComp, bAllReveal);
}

void UVisionPlayerStateComp::BeginPlay()
{
    Super::BeginPlay();
    if (UWorld* World = GetWorld())
        World->GetTimerManager().SetTimerForNextTick(this, &UVisionPlayerStateComp::RefreshVisibility);
}

// -------------------------------------------------------------------------- //
//  Team
// -------------------------------------------------------------------------- //

void UVisionPlayerStateComp::SetTeamChannel(EVisionChannel InTeam)
{
    TeamChannel = InTeam;

    UE_LOG(VisionPlayerStateComp, Log,
        TEXT("[%s] SetTeamChannel >> %d"), *GetOwner()->GetName(), (uint8)TeamChannel);

    RefreshVisibility();
}

void UVisionPlayerStateComp::OnRep_TeamChannel()
{
    UE_LOG(VisionPlayerStateComp, Log,
        TEXT("[%s] OnRep_TeamChannel >> %d"), *GetOwner()->GetName(), (uint8)TeamChannel);
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
        TEXT("[%s] SetAllReveal >> %s"), *GetOwner()->GetName(),
        bAllReveal ? TEXT("ON") : TEXT("OFF"));

    RefreshVisibility();
}

void UVisionPlayerStateComp::OnRep_AllReveal()
{
    UE_LOG(VisionPlayerStateComp, Log,
        TEXT("[%s] OnRep_AllReveal >> %s"), *GetOwner()->GetName(),
        bAllReveal ? TEXT("ON") : TEXT("OFF"));
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
//  ReevaluateTargetVisibility
//
//  This is the core fix.  It does NOT accept a bVisible hint from the caller.
//  Instead it reads the live VisibleActors list and scans every entry for
//  this target.  If ANY entry belongs to a team this player can see, the
//  actor is shown.  This means one team losing sight can never hide an
//  actor that another eligible team still sees.
// -------------------------------------------------------------------------- //

void UVisionPlayerStateComp::ReevaluateTargetVisibility(AActor* Target)
{
    if (!Target)
        return;

    UVision_VisualComp* VisualComp = Target->FindComponentByClass<UVision_VisualComp>();
    if (!VisualComp)
        return;

    // Only the local controller applies rendering changes.
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC || !PC->IsLocalController())
        return;

    bool bShouldBeVisible = false;

    if (bAllReveal)
    {
        bShouldBeVisible = true;
    }
    else
    {
        // Same-team and AlwaysVisible actors are visible regardless of LOS votes.
        const EVisionChannel TargetTeam = VisualComp->GetVisionChannel();
        if (CanSeeTeam(TargetTeam))
        {
            bShouldBeVisible = true;
        }
        else
        {
            // Scan every live entry for this target.
            // The entry that triggered this call may already be gone from the
            // array (PreReplicatedRemove fires before the item is removed on
            // client, but SetActorVisibleToTeam removes it before calling us
            // on server).  Either way the array reflects current truth.
            UVisionGameStateComp* GSComp = nullptr;
            if (AGameStateBase* GS = GetWorld()->GetGameState())
                GSComp = GS->FindComponentByClass<UVisionGameStateComp>();

            if (GSComp)
            {
                for (const FVisibleActorEntry& Entry : GSComp->GetVisibleActors())
                {
                    if (Entry.Target != Target)
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

    UE_LOG(VisionPlayerStateComp, Verbose,
        TEXT("[%s] ReevaluateTargetVisibility >> %s -> %s"),
        *GetOwner()->GetName(),
        *Target->GetName(),
        bShouldBeVisible ? TEXT("VISIBLE") : TEXT("HIDDEN"));

    VisualComp->SetVisible(bShouldBeVisible);
}

// -------------------------------------------------------------------------- //
//  RefreshVisibility
//
//  Collects unique targets from the live list and re-evaluates each once.
//  A single target may have multiple entries (one per team that sees it),
//  so deduplication avoids redundant SetVisible calls.
// -------------------------------------------------------------------------- //

void UVisionPlayerStateComp::RefreshVisibility()
{
    UWorld* World = GetWorld();
    if (!World) return;

    AGameStateBase* GS = World->GetGameState();
    if (!GS) return;

    UVisionGameStateComp* GSComp = GS->FindComponentByClass<UVisionGameStateComp>();
    if (!GSComp) return;

    // Drain anything queued before we were ready.
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