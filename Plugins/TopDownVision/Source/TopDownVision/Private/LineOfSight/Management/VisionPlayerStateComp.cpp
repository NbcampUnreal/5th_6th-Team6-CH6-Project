// Fill out your copyright notice in the Description page of Project Settings.

#include "LineOfSight/Management/VisionPlayerStateComp.h"

#include "Net/UnrealNetwork.h"

//Log
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

// -------------------------------------------------------------------------- //
//  Team
// -------------------------------------------------------------------------- //

void UVisionPlayerStateComp::SetTeamChannel(EVisionChannel InTeam)
{
    if (!GetOwner()->HasAuthority())
    {
        UE_LOG(VisionPlayerStateComp, Warning,
            TEXT("[%s] SetTeamChannel >> Only server can set team"),
            *GetOwner()->GetName());
        return;
    }

    TeamChannel = InTeam;

    UE_LOG(VisionPlayerStateComp, Log,
        TEXT("[%s] SetTeamChannel >> Team set to %d"),
        *GetOwner()->GetName(), (uint8)TeamChannel);
}

void UVisionPlayerStateComp::OnRep_TeamChannel()
{
    UE_LOG(VisionPlayerStateComp, Log,
        TEXT("[%s] OnRep_TeamChannel >> Team replicated: %d"),
        *GetOwner()->GetName(), (uint8)TeamChannel);
}

// -------------------------------------------------------------------------- //
//  All Reveal
// -------------------------------------------------------------------------- //

void UVisionPlayerStateComp::SetAllReveal(bool bEnabled)
{
    if (!GetOwner()->HasAuthority())
    {
        UE_LOG(VisionPlayerStateComp, Warning,
            TEXT("[%s] SetAllReveal >> Only server can set AllReveal"),
            *GetOwner()->GetName());
        return;
    }

    bAllReveal = bEnabled;

    UE_LOG(VisionPlayerStateComp, Log,
        TEXT("[%s] SetAllReveal >> AllReveal set to %s"),
        *GetOwner()->GetName(),
        bAllReveal ? TEXT("ON") : TEXT("OFF"));
}

void UVisionPlayerStateComp::OnRep_AllReveal()
{
    UE_LOG(VisionPlayerStateComp, Log,
        TEXT("[%s] OnRep_AllReveal >> AllReveal replicated: %s"),
        *GetOwner()->GetName(),
        bAllReveal ? TEXT("ON") : TEXT("OFF"));

    // TODO: when AllReveal turns on, force-reveal all currently tracked targets
    // when it turns off, re-evaluate visibility based on team
}

// -------------------------------------------------------------------------- //
//  Query
// -------------------------------------------------------------------------- //

bool UVisionPlayerStateComp::CanSeeTeam(EVisionChannel InTeam) const
{
    if (bAllReveal)
        return true;

    return TeamChannel == InTeam;
}