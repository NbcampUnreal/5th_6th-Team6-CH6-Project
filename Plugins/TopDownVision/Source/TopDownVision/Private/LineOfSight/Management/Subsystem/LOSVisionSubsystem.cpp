#include "LineOfSight/Management/Subsystem/LOSVisionSubsystem.h"

#include "TopDownVision/Public/LineOfSight/VisionComps/Vision_VisualComp.h"
#include "LineOfSight/Management/VisionGameStateComp.h"
#include "LineOfSight/Management/VisionPlayerStateComp.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"

DEFINE_LOG_CATEGORY(LOSVisionSubsystem);

// -------------------------------------------------------------------------- //
//  FTargetVisibilityVotes
// -------------------------------------------------------------------------- //

bool FTargetVisibilityVotes::HasAnyVisibleVoteForTeam(EVisionChannel Team) const
{
    for (const TPair<AActor*, bool>& Pair : VotesByObserver)
    {
        if (!Pair.Key || !Pair.Value)
            continue;

        UVision_VisualComp* VisualComp = Pair.Key->FindComponentByClass<UVision_VisualComp>();
        if (VisualComp && VisualComp->GetVisionChannel() == Team)
            return true;
    }
    return false;
}

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
        if (UVisionPlayerStateComp* VPS = PC->PlayerState->FindComponentByClass<UVisionPlayerStateComp>())
            return VPS;
    }

    if (AGameStateBase* GS = World->GetGameState())
    {
        for (APlayerState* PS : GS->PlayerArray)
        {
            if (PS && PS->GetOwningController() == PC)
            {
                if (UVisionPlayerStateComp* VPS = PS->FindComponentByClass<UVisionPlayerStateComp>())
                    return VPS;
            }
        }
    }

    return nullptr;
}

// -------------------------------------------------------------------------- //
//  Provider registration
// -------------------------------------------------------------------------- //

bool ULOSVisionSubsystem::RegisterProvider(UVision_VisualComp* Provider, EVisionChannel InVisionChannel)
{
    if (!Provider)
    {
        UE_LOG(LOSVisionSubsystem, Error, TEXT("RegisterProvider >> Null provider"));
        return false;
    }
    if (InVisionChannel == EVisionChannel::None)
    {
        UE_LOG(LOSVisionSubsystem, Error, TEXT("RegisterProvider >> VisionChannel is None"));
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

    UE_LOG(LOSVisionSubsystem, Log,
        TEXT("RegisterProvider >> %s registered on channel %d"),
        *Provider->GetOwner()->GetName(), (uint8)InVisionChannel);

    HandleProviderRegistered(Provider, InVisionChannel);
    return true;
}

void ULOSVisionSubsystem::UnregisterProvider(UVision_VisualComp* Provider, EVisionChannel InVisionChannel)
{
    if (!Provider)
    {
        UE_LOG(LOSVisionSubsystem, Error, TEXT("UnregisterProvider >> Null provider"));
        return;
    }

    if (FRegisteredProviders* ChannelEntry = VisionMap.Find(InVisionChannel))
    {
        if (ChannelEntry->RegisteredList.Remove(Provider) > 0)
        {
            UE_LOG(LOSVisionSubsystem, Log,
                TEXT("UnregisterProvider >> %s unregistered from channel %d"),
                *Provider->GetOwner()->GetName(), (uint8)InVisionChannel);
            return;
        }
    }

    UE_LOG(LOSVisionSubsystem, Warning,
        TEXT("UnregisterProvider >> Could not find %s on channel %d"),
        *Provider->GetOwner()->GetName(), (uint8)InVisionChannel);
}

TArray<UVision_VisualComp*> ULOSVisionSubsystem::GetProvidersForTeam(EVisionChannel TeamChannel) const
{
    TArray<UVision_VisualComp*> Out;

    if (const FRegisteredProviders* Entry = VisionMap.Find(TeamChannel))
        Out.Append(Entry->RegisteredList);
    else
        UE_LOG(LOSVisionSubsystem, Verbose,
            TEXT("GetProvidersForTeam >> No providers yet for channel %d"), (uint8)TeamChannel);

    return Out;
}

TArray<UVision_VisualComp*> ULOSVisionSubsystem::GeAllProviders() const
{
    TArray<UVision_VisualComp*> Out;

    for (const TPair<EVisionChannel, FRegisteredProviders>& Pair : VisionMap)
        Out.Append(Pair.Value.RegisteredList);

    return Out;
}

// -------------------------------------------------------------------------- //
//  Provider reveal logic
// -------------------------------------------------------------------------- //

void ULOSVisionSubsystem::HandleProviderRegistered(UVision_VisualComp* NewProvider, EVisionChannel Channel)
{
    UVisionGameStateComp* GSComp = GetVisionGameStateComp();
    if (!GSComp)
        return;

    for (UVision_VisualComp* Existing : GetProvidersForTeam(Channel))
    {
        if (!Existing || !Existing->GetOwner())
            continue;

        GSComp->SetActorVisibleToTeam(Existing->GetOwner(), Channel);

        UE_LOG(LOSVisionSubsystem, Log,
            TEXT("HandleProviderRegistered >> Revealed %s to channel [%s]"),
            *Existing->GetOwner()->GetName(), *UEnum::GetValueAsString(Channel));
    }

    if (UVisionPlayerStateComp* VisionPS = GetLocalVisionPS(GetWorld()))
    {
        UE_LOG(LOSVisionSubsystem, Log,
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
// -------------------------------------------------------------------------- //

void ULOSVisionSubsystem::ReportTargetVisibilityFromClient(
    AActor* Observer, EVisionChannel ObserverTeam, AActor* Target, bool bVisible)
{
    if (!Observer || !Target || ObserverTeam == EVisionChannel::None)
        return;

    if (GetWorld()->GetNetMode() == NM_Standalone)
    {
        ReportTargetVisibility(Observer, ObserverTeam, Target, bVisible);
    }
    else
    {
        UVisionPlayerStateComp* VisionPS = GetLocalVisionPS(GetWorld());
        if (!VisionPS)
        {
            UE_LOG(LOSVisionSubsystem, Warning,
                TEXT("ReportTargetVisibilityFromClient >> No local VisionPS found"));
            return;
        }
        VisionPS->Server_ReportVisibility(Observer, Target, ObserverTeam, bVisible);
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
    {
        UE_LOG(LOSVisionSubsystem, Warning,
            TEXT("ReportTargetVisibility >> No VisionGameStateComp"));
        return;
    }

    FTargetVisibilityVotes& Votes = VisibilityVotes.FindOrAdd(Target);

    // Skip if this observer's vote hasn't changed
    const bool* LastVote = Votes.VotesByObserver.Find(Observer);
    if (LastVote && *LastVote == bVisible)
        return;

    const bool bWasVisible = Votes.HasAnyVisibleVoteForTeam(ObserverTeam);

    Votes.VotesByObserver.FindOrAdd(Observer) = bVisible;

    const bool bIsNowVisible = Votes.HasAnyVisibleVoteForTeam(ObserverTeam);

    UE_LOG(LOSVisionSubsystem, Verbose,
        TEXT("ReportTargetVisibility >> %s | Observer:%s | Team:%s | Vote:%s | Was:%s | Now:%s"),
        *Target->GetName(),
        *Observer->GetName(),
        *UEnum::GetValueAsString(ObserverTeam),
        bVisible      ? TEXT("Y") : TEXT("N"),
        bWasVisible   ? TEXT("Y") : TEXT("N"),
        bIsNowVisible ? TEXT("Y") : TEXT("N"));

    if (!bWasVisible && bIsNowVisible)
        GSComp->SetActorVisibleToTeam(Target, ObserverTeam);
    else if (bWasVisible && !bIsNowVisible)
        GSComp->ClearActorVisibleToTeam(Target, ObserverTeam);
}

// -------------------------------------------------------------------------- //
//  Debug
// -------------------------------------------------------------------------- //

void ULOSVisionSubsystem::PrintVisionStatus() const
{
    UE_LOG(LOSVisionSubsystem, Log, TEXT("========== Vision Status =========="));

    // Collect all channels present in the vote map
    TMap<EVisionChannel, TArray<FString>> VisiblePerTeam;

    for (const TPair<AActor*, FTargetVisibilityVotes>& TargetPair : VisibilityVotes)
    {
        AActor* Target = TargetPair.Key;
        if (!Target) continue;

        // Check each registered channel
        for (const TPair<EVisionChannel, FRegisteredProviders>& ChannelPair : VisionMap)
        {
            const EVisionChannel Channel = ChannelPair.Key;
            if (TargetPair.Value.HasAnyVisibleVoteForTeam(Channel))
            {
                VisiblePerTeam.FindOrAdd(Channel).Add(Target->GetName());
            }
        }
    }

    if (VisiblePerTeam.IsEmpty())
    {
        UE_LOG(LOSVisionSubsystem, Log, TEXT("  No visibility data yet"));
    }
    else
    {
        for (const TPair<EVisionChannel, TArray<FString>>& Pair : VisiblePerTeam)
        {
            UE_LOG(LOSVisionSubsystem, Log,
                TEXT("  %s seeing [%d]: %s"),
                *UEnum::GetValueAsString(Pair.Key),
                Pair.Value.Num(),
                *FString::Join(Pair.Value, TEXT(", ")));
        }
    }

    UE_LOG(LOSVisionSubsystem, Log, TEXT("==================================="));
}

// -------------------------------------------------------------------------- //

UVisionGameStateComp* ULOSVisionSubsystem::GetVisionGameStateComp() const
{
    AGameStateBase* GS = GetWorld()->GetGameState();
    if (!GS)
    {
        UE_LOG(LOSVisionSubsystem, Warning, TEXT("GetVisionGameStateComp >> No GameState"));
        return nullptr;
    }

    UVisionGameStateComp* Comp = GS->FindComponentByClass<UVisionGameStateComp>();
    if (!Comp)
    {
        UE_LOG(LOSVisionSubsystem, Warning,
            TEXT("GetVisionGameStateComp >> Not found on %s"), *GS->GetClass()->GetName());
    }

    return Comp;
}