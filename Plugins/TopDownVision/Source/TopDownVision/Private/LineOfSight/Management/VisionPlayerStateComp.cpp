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
    // Deferred one tick — by then PC->PlayerState is assigned and GSComp is populated
    if (UWorld* World = GetWorld())
        World->GetTimerManager().SetTimerForNextTick(this, &UVisionPlayerStateComp::RefreshVisibility);
}

// -------------------------------------------------------------------------- //
//  Team
// -------------------------------------------------------------------------- //

void UVisionPlayerStateComp::SetTeamChannel(EVisionChannel InTeam)
{
    /*if (!GetOwner()->HasAuthority())
    {
        UE_LOG(VisionPlayerStateComp, Warning,
            TEXT("[%s] SetTeamChannel >> Server only"), *GetOwner()->GetName());
        return;
    }*/

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
//  Visibility logic — single place for filter + apply
// -------------------------------------------------------------------------- //

bool UVisionPlayerStateComp::CanSeeTeam(EVisionChannel InTeam) const
{
    /*return bAllReveal || (TeamChannel == InTeam);*/
    // AlwaysVisible bypasses all team filtering
    /*if (InTeam == EVisionChannel::AlwaysVisible)
        return true;

    if (bAllReveal)
        return true;

    if (TeamChannel == EVisionChannel::None)
        return false;

    return InTeam == TeamChannel
        || InTeam == EVisionChannel::SharedVision;*/
    /*if (InTeam == EVisionChannel::None)          return false; 
    if (InTeam == EVisionChannel::AlwaysVisible) return true;
    if (bAllReveal)                              return true;
    if (TeamChannel == EVisionChannel::None)     return false;

    return InTeam == TeamChannel
        || InTeam == EVisionChannel::SharedVision;*/

    if (InTeam == EVisionChannel::AlwaysVisible) // always
        return true;

    return bAllReveal || (TeamChannel == InTeam);
}

void UVisionPlayerStateComp::ApplyActorVisibility(AActor* Target, EVisionChannel Team, bool bVisible)
{
    if (!Target)
        return;

    UVision_VisualComp* VisualComp = Target->FindComponentByClass<UVision_VisualComp>();
    if (!VisualComp)
        return;

    // Check the target's actual team, not the observer's team
    const bool bTargetIsSameTeam = (VisualComp->GetVisionChannel() == TeamChannel);

    const bool bShouldBeVisible = bAllReveal
        || bTargetIsSameTeam                    // same team — always visible regardless of LOS
        || (CanSeeTeam(Team) && bVisible);       // enemy — only visible if my team spotted them and LOS confirms

    VisualComp->SetVisible(bShouldBeVisible);

    UE_LOG(VisionPlayerStateComp, Verbose,
        TEXT("[%s] ApplyActorVisibility >> %s | ObserverTeam:%s | TargetTeam:%s | SameTeam:%d | bVisible:%d | Result:%d"),
        *GetOwner()->GetName(),
        *Target->GetName(),
        *UEnum::GetValueAsString(Team),
        *UEnum::GetValueAsString(VisualComp->GetVisionChannel()),
        bTargetIsSameTeam,
        bVisible,
        bShouldBeVisible);
}

// -------------------------------------------------------------------------- //
//  Full refresh — initial sync and on team/AllReveal change
// -------------------------------------------------------------------------- //

void UVisionPlayerStateComp::RefreshVisibility()
{
    UWorld* World = GetWorld();
    if (!World) return;

    AGameStateBase* GS = World->GetGameState();
    if (!GS) return;

    UVisionGameStateComp* GSComp = GS->FindComponentByClass<UVisionGameStateComp>();
    if (!GSComp) return;

    // Drain anything that arrived before we were ready
    GSComp->FlushPendingReveals(this);

    UE_LOG(VisionPlayerStateComp, Log,
        TEXT("[%s] RefreshVisibility >> %d actors | Team:%d | AllReveal:%d"),
        *GetOwner()->GetName(),
        GSComp->GetVisibleActors().Num(),
        (uint8)TeamChannel,
        bAllReveal);

    for (const FVisibleActorEntry& Entry : GSComp->GetVisibleActors())
    {
        if (Entry.Target)
            ApplyActorVisibility(Entry.Target, Entry.TeamChannel, true);
    }
}