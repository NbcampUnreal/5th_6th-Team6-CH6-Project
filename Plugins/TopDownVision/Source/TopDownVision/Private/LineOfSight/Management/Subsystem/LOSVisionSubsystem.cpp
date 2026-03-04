// Fill out your copyright notice in the Description page of Project Settings.

#include "LineOfSight/Management/Subsystem/LOSVisionSubsystem.h"

#include "TopDownVision/Public/LineOfSight/VisionComps/Vision_VisualComp.h"
#include "LineOfSight/Management/VisionGameStateComp.h"
#include "GameFramework/GameStateBase.h"

DEFINE_LOG_CATEGORY(LOSVisionSubsystem);

// Replace RegisterProvider in LOSVisionSubsystem.cpp with this:

bool ULOSVisionSubsystem::RegisterProvider(UVision_VisualComp* Provider, EVisionChannel InVisionChannel)
{
    if (!Provider)
    {
        UE_LOG(LOSVisionSubsystem, Error,
            TEXT("ULOSVisionSubsystem::RegisterProvider >> Invalid provider"));
        return false;
    }
    if (InVisionChannel == EVisionChannel::None)
    {
        UE_LOG(LOSVisionSubsystem, Error,
            TEXT("ULOSVisionSubsystem::RegisterProvider >> VisionChannel not settled"));
        return false;
    }

    FRegisteredProviders& ChannelEntry = VisionMap.FindOrAdd(InVisionChannel);
    if (ChannelEntry.RegisteredList.Contains(Provider))
    {
        UE_LOG(LOSVisionSubsystem, Warning,
            TEXT("ULOSVisionSubsystem::RegisterProvider >> Already registered provider[%s] in channel:%d"),
            *Provider->GetOwner()->GetName(), (uint8)InVisionChannel);
        return false;
    }

    ChannelEntry.RegisteredList.Add(Provider);
    UE_LOG(LOSVisionSubsystem, Log,
        TEXT("ULOSVisionSubsystem::RegisterProvider >> Provider[%s] registered. channel:%d"),
        *Provider->GetOwner()->GetName(), (uint8)InVisionChannel);

    // Notify GameStateComp to reveal all same-team providers to each other
    if (UVisionGameStateComp* GSComp = GetVisionGameStateComp())
        GSComp->OnProviderRegistered(Provider, InVisionChannel);

    return true;
}


void ULOSVisionSubsystem::UnregisterProvider(UVision_VisualComp* Provider, EVisionChannel InVisionChannel)
{
    if (!Provider)
    {
        UE_LOG(LOSVisionSubsystem, Error,
            TEXT("ULOSVisionSubsystem::UnregisterProvider >> Invalid provider"));
        return;
    }

    if (FRegisteredProviders* ChannelEntry = VisionMap.Find(InVisionChannel))
    {
        if (ChannelEntry->RegisteredList.Remove(Provider) > 0)
        {
            UE_LOG(LOSVisionSubsystem, Log,
                TEXT("ULOSVisionSubsystem::UnregisterProvider >> Provider[%s] unregistered from channel:%d"),
                *Provider->GetOwner()->GetName(), (uint8)InVisionChannel);
            return;
        }
    }

    UE_LOG(LOSVisionSubsystem, Warning,
        TEXT("ULOSVisionSubsystem::UnregisterProvider >> Could not find Provider[%s] in channel:%d"),
        *Provider->GetOwner()->GetName(), (uint8)InVisionChannel);
}

TArray<UVision_VisualComp*> ULOSVisionSubsystem::GetProvidersForTeam(EVisionChannel TeamChannel) const
{
    TArray<UVision_VisualComp*> OutProviders;

    if (const FRegisteredProviders* TeamEntry = VisionMap.Find(TeamChannel))
        OutProviders.Append(TeamEntry->RegisteredList);
    else
        UE_LOG(LOSVisionSubsystem, Error,
            TEXT("ULOSVisionSubsystem::GetProvidersForTeam >> No providers for channel:%d"),
            (uint8)TeamChannel);

    return OutProviders;
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
            TEXT("ReportTargetVisibility >> No VisionGameStateComp found on GameState"));
        return;
    }

    const uint8 TeamID = (uint8)ObserverTeam;

    // Get or create vote entry for this target
    FTargetVisibilityVotes& Votes = VisibilityVotes.FindOrAdd(Target);
    int32& VoteCount = Votes.VotesByTeam.FindOrAdd(TeamID, 0);

    const bool bWasVisible = VoteCount > 0;

    // Update vote count
    if (bVisible)
        VoteCount = FMath::Max(VoteCount + 1, 1);
    else
        VoteCount = FMath::Max(VoteCount - 1, 0);

    const bool bIsNowVisible = VoteCount > 0;

    UE_LOG(LOSVisionSubsystem, Verbose,
        TEXT("ReportTargetVisibility >> Target: %s | Team: %d | Votes: %d | WasVisible: %s | IsVisible: %s"),
        *Target->GetName(), TeamID, VoteCount,
        bWasVisible    ? TEXT("YES") : TEXT("NO"),
        bIsNowVisible  ? TEXT("YES") : TEXT("NO"));

    // Only update GameStateComp when state actually changes
    if (!bWasVisible && bIsNowVisible)
    {
        GSComp->SetActorVisibleToTeam(Target, ObserverTeam);
    }
    else if (bWasVisible && !bIsNowVisible)
    {
        GSComp->ClearActorVisibleToTeam(Target, ObserverTeam);
    }
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
            TEXT("GetVisionGameStateComp >> VisionGameStateComp not found on %s"),
            *GS->GetClass()->GetName());
    }

    return Comp;
}