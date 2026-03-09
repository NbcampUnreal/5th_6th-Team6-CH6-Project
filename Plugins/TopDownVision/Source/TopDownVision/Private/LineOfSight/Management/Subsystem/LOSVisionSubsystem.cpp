#include "LineOfSight/Management/Subsystem/LOSVisionSubsystem.h"

#include "TopDownVision/Public/LineOfSight/VisionComps/Vision_VisualComp.h"
#include "LineOfSight/Management/VisionGameStateComp.h"
#include "LineOfSight/Management/VisionPlayerStateComp.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"

DEFINE_LOG_CATEGORY(LOSVisionSubsystem);

// -------------------------------------------------------------------------- //
//  Local player lookup — single authoritative location, used by GameStateComp too
// -------------------------------------------------------------------------- //

UVisionPlayerStateComp* ULOSVisionSubsystem::GetLocalVisionPS(UWorld* World)
{
    if (!World)
        return nullptr;

    APlayerController* PC = GEngine->GetFirstLocalPlayerController(World);
    if (!PC)
        return nullptr;

    // Fast path
    if (PC->PlayerState)
    {
        if (UVisionPlayerStateComp* VPS = PC->PlayerState->FindComponentByClass<UVisionPlayerStateComp>())
            return VPS;
    }

    // Fallback — PC->PlayerState may be null during early BeginPlay
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

    UE_LOG(LOSVisionSubsystem, Log,
        TEXT("RegisterProvider >> %s registered on channel %d"),
        *Provider->GetOwner()->GetName(), (uint8)InVisionChannel);

    // All reveal/refresh logic lives here — not in GameStateComp
    HandleProviderRegistered(Provider, InVisionChannel);

    return true;
}

void ULOSVisionSubsystem::UnregisterProvider(UVision_VisualComp* Provider, EVisionChannel InVisionChannel)
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

TArray<UVision_VisualComp*> ULOSVisionSubsystem::GeAlltProviders() const
{
    TArray<UVision_VisualComp*> Out;

    for (const TPair<EVisionChannel, FRegisteredProviders>& Pair : VisionMap)
    {
        Out.Append(Pair.Value.RegisteredList);
    }

    return Out;
}

// -------------------------------------------------------------------------- //
//  Provider reveal logic — was OnProviderRegistered in GameStateComp
// -------------------------------------------------------------------------- //

void ULOSVisionSubsystem::HandleProviderRegistered(UVision_VisualComp* NewProvider, EVisionChannel Channel)
{
    UVisionGameStateComp* GSComp = GetVisionGameStateComp();
    if (!GSComp)
        return;

    // Reveal every same-team actor already tracked (including the new one itself)
    for (UVision_VisualComp* Existing : GetProvidersForTeam(Channel))
    {
        if (!Existing || !Existing->GetOwner())
            continue;

        GSComp->SetActorVisibleToTeam(Existing->GetOwner(), Channel);

        UE_LOG(LOSVisionSubsystem, Log,
            TEXT("HandleProviderRegistered >> Revealed %s to channel [%s]"),
            *Existing->GetOwner()->GetName(), *UEnum::GetValueAsString(Channel));
    }

    // Refresh local player if available.
    // If VisionPS is still null here, VisionPlayerStateComp::BeginPlay
    // schedules RefreshVisibility for next tick — that path is the safety net.
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

    const uint8 TeamID = (uint8)ObserverTeam;

    FTargetVisibilityVotes& Votes = VisibilityVotes.FindOrAdd(Target);
    int32& VoteCount = Votes.VotesByTeam.FindOrAdd(TeamID, 0);

    const bool bWasVisible = VoteCount > 0;

    VoteCount = bVisible
        ? FMath::Max(VoteCount + 1, 1)
        : FMath::Max(VoteCount - 1, 0);

    const bool bIsNowVisible = VoteCount > 0;

    UE_LOG(LOSVisionSubsystem, Verbose,
        TEXT("ReportTargetVisibility >> %s | Team:%d | Votes:%d | Was:%s | Now:%s"),
        *Target->GetName(), TeamID, VoteCount,
        bWasVisible   ? TEXT("Y") : TEXT("N"),
        bIsNowVisible ? TEXT("Y") : TEXT("N"));

    if (!bWasVisible && bIsNowVisible)
        GSComp->SetActorVisibleToTeam(Target, ObserverTeam);
    else if (bWasVisible && !bIsNowVisible)
        GSComp->ClearActorVisibleToTeam(Target, ObserverTeam);
}

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