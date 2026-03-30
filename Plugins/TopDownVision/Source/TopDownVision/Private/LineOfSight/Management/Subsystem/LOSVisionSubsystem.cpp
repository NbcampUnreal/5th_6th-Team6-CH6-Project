#include "LineOfSight/Management/Subsystem/LOSVisionSubsystem.h"

#include "TopDownVision/Public/LineOfSight/VisionComps/Vision_VisualComp.h"
#include "LineOfSight/Management/VisionGameStateComp.h"
#include "LineOfSight/Management/VisionPlayerStateComp.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"

DEFINE_LOG_CATEGORY(LOSVisionSubsystem);

// -------------------------------------------------------------------------- //
//  Local player lookup
// -------------------------------------------------------------------------- //

UVisionPlayerStateComp* ULOSVisionSubsystem::GetLocalVisionPS(UWorld* World)
{
    if (!World)
        return nullptr;

    APlayerController* PC = GEngine->GetFirstLocalPlayerController(World);
    if (!PC)
        return nullptr;

    if (PC->PlayerState)
    {
        if (UVisionPlayerStateComp* VPS =
            PC->PlayerState->FindComponentByClass<UVisionPlayerStateComp>())
            return VPS;
    }

    // Fallback — PC->PlayerState may be null during early BeginPlay
    if (AGameStateBase* GS = World->GetGameState())
    {
        for (APlayerState* PS : GS->PlayerArray)
        {
            if (PS && PS->GetOwningController() == PC)
            {
                if (UVisionPlayerStateComp* VPS =
                    PS->FindComponentByClass<UVisionPlayerStateComp>())
                    return VPS;
            }
        }
    }

    return nullptr;
}

// -------------------------------------------------------------------------- //
//  Provider registration
// -------------------------------------------------------------------------- //

bool ULOSVisionSubsystem::RegisterProvider(
    UVision_VisualComp* Provider, EVisionChannel InVisionChannel)
{
    if (!Provider)
    {
        UE_LOG(LOSVisionSubsystem, Error,
            TEXT("RegisterProvider >> Null provider"));
        return false;
    }
    if (InVisionChannel == EVisionChannel::None)
    {
        UE_LOG(LOSVisionSubsystem, Error,
            TEXT("RegisterProvider >> VisionChannel is None"));
        return false;
    }

    FRegisteredProviders& ChannelEntry = VisionMap.FindOrAdd(InVisionChannel);
    if (ChannelEntry.RegisteredList.Contains(Provider))
    {
        UE_LOG(LOSVisionSubsystem, Warning,
            TEXT("RegisterProvider >> Already registered: %s on channel %d"),
            *Provider->GetOwner()->GetName(), (uint8)InVisionChannel);
        return false;
    }

    ChannelEntry.RegisteredList.Add(Provider);

    UE_LOG(LOSVisionSubsystem, Verbose,
        TEXT("RegisterProvider >> %s registered on channel %d"),
        *Provider->GetOwner()->GetName(), (uint8)InVisionChannel);

    HandleProviderRegistered(Provider, InVisionChannel);
    return true;
}

void ULOSVisionSubsystem::UnregisterProvider(
    UVision_VisualComp* Provider, EVisionChannel InVisionChannel)
{
    if (!Provider)
    {
        UE_LOG(LOSVisionSubsystem, Error,
            TEXT("UnregisterProvider >> Null provider"));
        return;
    }

    if (FRegisteredProviders* ChannelEntry = VisionMap.Find(InVisionChannel))
    {
        if (ChannelEntry->RegisteredList.Remove(Provider) > 0)
        {
            UE_LOG(LOSVisionSubsystem, Verbose,
                TEXT("UnregisterProvider >> %s unregistered from channel %d"),
                *Provider->GetOwner()->GetName(), (uint8)InVisionChannel);
            return;
        }
    }

    UE_LOG(LOSVisionSubsystem, Warning,
        TEXT("UnregisterProvider >> Could not find %s on channel %d"),
        *Provider->GetOwner()->GetName(), (uint8)InVisionChannel);
}

TArray<UVision_VisualComp*> ULOSVisionSubsystem::GetProvidersForTeam(
    EVisionChannel TeamChannel) const
{
    TArray<UVision_VisualComp*> Out;

    if (const FRegisteredProviders* Entry = VisionMap.Find(TeamChannel))
        Out.Append(Entry->RegisteredList);
    else
        UE_LOG(LOSVisionSubsystem, Verbose,
            TEXT("GetProvidersForTeam >> No providers yet for channel %d"),
            (uint8)TeamChannel);

    return Out;
}

// Fixed typo: was GeAllProviders
TArray<UVision_VisualComp*> ULOSVisionSubsystem::GetAllProviders() const
{
    TArray<UVision_VisualComp*> Out;
    for (const TPair<EVisionChannel, FRegisteredProviders>& Pair : VisionMap)
        Out.Append(Pair.Value.RegisteredList);
    return Out;
}

TArray<UVision_VisualComp*> ULOSVisionSubsystem::GetProvidersVisibleToLocalPlayer() const
{
    TArray<UVision_VisualComp*> AllProviders = GetAllProviders();

    UVisionPlayerStateComp* LocalVisionPS = GetLocalVisionPS(GetWorld());
    if (!LocalVisionPS)
    {
        // VisionPS not ready yet — return everything so the RT is not left
        // blank. RefreshVisibility will correct state on the next tick.
        UE_LOG(LOSVisionSubsystem, Verbose,
            TEXT("GetProvidersVisibleToLocalPlayer >> VisionPS not ready, returning all providers"));
        return AllProviders;
    }

    TArray<UVision_VisualComp*> Out;
    Out.Reserve(AllProviders.Num());

    for (UVision_VisualComp* Provider : AllProviders)
    {
        if (!Provider || !Provider->GetOwner())
            continue;

        if (LocalVisionPS->CanSeeTeam(Provider->GetVisionChannel()))
            Out.Add(Provider);
    }

    return Out;
}

// -------------------------------------------------------------------------- //
//  Provider reveal logic
// -------------------------------------------------------------------------- //

void ULOSVisionSubsystem::HandleProviderRegistered(
    UVision_VisualComp* NewProvider, EVisionChannel Channel)
{
    UVisionGameStateComp* GSComp = GetVisionGameStateComp();
    if (!GSComp)
        return;

    for (UVision_VisualComp* Existing : GetProvidersForTeam(Channel))
    {
        if (!Existing || !Existing->GetOwner())
            continue;

        // Fixed: was SetActorVisibleByTeam
        GSComp->SetActorVisibleToTeam(Existing->GetOwner(), Channel);

        UE_LOG(LOSVisionSubsystem, Verbose,
            TEXT("HandleProviderRegistered >> Revealed %s to channel [%s]"),
            *Existing->GetOwner()->GetName(), *UEnum::GetValueAsString(Channel));
    }

    if (UVisionPlayerStateComp* VisionPS = GetLocalVisionPS(GetWorld()))
    {
        UE_LOG(LOSVisionSubsystem, Verbose,
            TEXT("HandleProviderRegistered >> VisionPS ready, calling RefreshVisibility"));
        VisionPS->RefreshVisibility();
    }
    else
    {
        UE_LOG(LOSVisionSubsystem, Verbose,
            TEXT("HandleProviderRegistered >> VisionPS not ready, BeginPlay next-tick will catch up"));
    }
}

// -------------------------------------------------------------------------- //
//  Visibility reporting
//
//  Uses a TSet of weak observer pointers per team instead of a vote counter.
//  This makes add/remove idempotent — the same observer reporting twice does
//  not inflate the count, and a stale observer (destroyed pawn) is cleaned up
//  automatically without needing a matching Remove call.
// -------------------------------------------------------------------------- //

static void CleanInvalidObservers(TSet<TWeakObjectPtr<AActor>>& Set)
{
    for (auto It = Set.CreateIterator(); It; ++It)
    {
        if (!It->IsValid())
            It.RemoveCurrent();
    }
}

void ULOSVisionSubsystem::ReportTargetVisibility(
    AActor* Observer,
    EVisionChannel ObserverTeam,
    AActor* Target,
    bool bVisible)
{
    if (!Observer || !Target || ObserverTeam == EVisionChannel::None)
        return;

    UVisionGameStateComp* GSComp = GetVisionGameStateComp();
    if (!GSComp)
        return;

    const uint8 TeamID = (uint8)ObserverTeam;

    FTargetVisibilityVotes& Votes     = VisibilityVotes.FindOrAdd(Target);
    TSet<TWeakObjectPtr<AActor>>& Observers = Votes.ObserversByTeam.FindOrAdd(TeamID);

    // Clean stale weak pointers before reading Num()
    CleanInvalidObservers(Observers);

    const bool bWasVisible = Observers.Num() > 0;

    if (bVisible)
        Observers.Add(Observer);
    else
        Observers.Remove(Observer);

    // Clean again after removal in case the observer was already stale
    CleanInvalidObservers(Observers);

    const bool bIsNowVisible = Observers.Num() > 0;

    // Prune empty entries so the map doesn't grow unboundedly
    if (Observers.Num() == 0)
    {
        Votes.ObserversByTeam.Remove(TeamID);

        if (Votes.ObserversByTeam.Num() == 0)
            VisibilityVotes.Remove(Target);
    }

    UE_LOG(LOSVisionSubsystem, Verbose,
        TEXT("[VISION] %s | Team:%d | Observers:%d | Was:%d -> Now:%d"),
        *Target->GetName(), TeamID, Observers.Num(), bWasVisible, bIsNowVisible);

    // Fixed: was SetActorVisibleByTeam
    if (!bWasVisible && bIsNowVisible)
        GSComp->SetActorVisibleToTeam(Target, ObserverTeam);
    else if (bWasVisible && !bIsNowVisible)
        GSComp->ClearActorVisibleToTeam(Target, ObserverTeam);
}

// -------------------------------------------------------------------------- //

UVisionGameStateComp* ULOSVisionSubsystem::GetVisionGameStateComp() const
{
    AGameStateBase* GS = GetWorld()->GetGameState();
    if (!GS)
    {
        UE_LOG(LOSVisionSubsystem, Warning,
            TEXT("GetVisionGameStateComp >> No GameState"));
        return nullptr;
    }

    UVisionGameStateComp* Comp = GS->FindComponentByClass<UVisionGameStateComp>();
    if (!Comp)
    {
        UE_LOG(LOSVisionSubsystem, Warning,
            TEXT("GetVisionGameStateComp >> Not found on %s"),
            *GS->GetClass()->GetName());
    }

    return Comp;
}