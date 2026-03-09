#include "LineOfSight/Management/VisionPlayerStateComp.h"

#include "Net/UnrealNetwork.h"
#include "GameFramework/GameStateBase.h"
#include "LineOfSight/Management/VisionGameStateComp.h"
#include "LineOfSight/Management/Subsystem/LOSVisionSubsystem.h"
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
        TEXT("[%s] SetTeamChannel >> %s"), *GetOwner()->GetName(),
        *UEnum::GetValueAsString(TeamChannel));

    RefreshVisibility();
}

void UVisionPlayerStateComp::OnRep_TeamChannel()
{
    UE_LOG(VisionPlayerStateComp, Log,
        TEXT("[%s] OnRep_TeamChannel >> %s"), *GetOwner()->GetName(),
        *UEnum::GetValueAsString(TeamChannel));
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
//  Visibility logic
// -------------------------------------------------------------------------- //

bool UVisionPlayerStateComp::CanSeeTeam(EVisionChannel InTeam) const
{
    return bAllReveal || (TeamChannel == InTeam);
}

void UVisionPlayerStateComp::ApplyActorVisibility(AActor* Target, EVisionChannel Team, bool bVisible)
{
    if (!Target)
        return;

    UVision_VisualComp* VisualComp = Target->FindComponentByClass<UVision_VisualComp>();
    if (!VisualComp)
        return;

    const bool bTargetIsSameTeam = (VisualComp->GetVisionChannel() == TeamChannel);

    const bool bShouldBeVisible = bAllReveal
        || bTargetIsSameTeam
        || (CanSeeTeam(Team) && bVisible);

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
//  Full refresh
// -------------------------------------------------------------------------- //

void UVisionPlayerStateComp::RefreshVisibility()
{
    UWorld* World = GetWorld();
    if (!World) return;

    AGameStateBase* GS = World->GetGameState();
    if (!GS) return;

    UVisionGameStateComp* GSComp = GS->FindComponentByClass<UVisionGameStateComp>();
    if (!GSComp) return;

    if (TeamChannel == EVisionChannel::None)
    {
        UE_LOG(VisionPlayerStateComp, Verbose,
            TEXT("[%s] RefreshVisibility >> TeamChannel is None, skipping"),
            *GetOwner()->GetName());
        return;
    }

    GSComp->FlushPendingReveals(this);

    for (const FVisibleActorEntry& Entry : GSComp->GetVisibleActors())
    {
        if (Entry.Target)
            ApplyActorVisibility(Entry.Target, Entry.TeamChannel, true);
    }

    if (!bVisionReady)
    {
        bVisionReady = true;
        OnVisionReady.Broadcast();
        UE_LOG(VisionPlayerStateComp, Log,
            TEXT("[%s] RefreshVisibility >> Vision is now ready, delegate fired"),
            *GetOwner()->GetName());
    }
}

// -------------------------------------------------------------------------- //
//  RPC — routes through PlayerState since client always owns it
// -------------------------------------------------------------------------- //

void UVisionPlayerStateComp::Server_ReportVisibility_Implementation(
    AActor* Observer, AActor* Target, EVisionChannel Channel, bool bVisible)
{
    UE_LOG(VisionPlayerStateComp, Warning,
        TEXT("[%s] Server_ReportVisibility >> Observer:%s | Target:%s | Channel:%s | Visible:%d"),
        *GetOwner()->GetName(),
        Observer ? *Observer->GetName() : TEXT("NULL"),
        Target   ? *Target->GetName()   : TEXT("NULL"),
        *UEnum::GetValueAsString(Channel),
        bVisible);

    if (!Observer || !Target)
    {
        UE_LOG(VisionPlayerStateComp, Warning,
            TEXT("[%s] Server_ReportVisibility >> Null actor in RPC — ensure both Observer and Target have bReplicates=true"),
            *GetOwner()->GetName());
        return;
    }

    ULOSVisionSubsystem* Subsystem = GetWorld()->GetSubsystem<ULOSVisionSubsystem>();
    if (!Subsystem) return;

    Subsystem->ReportTargetVisibility(Observer, Channel, Target, bVisible);
}