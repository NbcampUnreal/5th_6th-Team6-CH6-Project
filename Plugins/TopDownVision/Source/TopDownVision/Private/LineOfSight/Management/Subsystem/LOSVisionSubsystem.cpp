// Fill out your copyright notice in the Description page of Project Settings.


#include "LineOfSight/Management/Subsystem/LOSVisionSubsystem.h"

#include "LineOfSight/LineOfSightComponent.h"


//Log
DEFINE_LOG_CATEGORY(LOSVisionSubsystem);

bool ULOSVisionSubsystem::RegisterProvider(ULineOfSightComponent* Provider, EVisionChannel InVisionChannel)
{
	if (!Provider)
	{
		UE_LOG(LOSVisionSubsystem, Error,
			TEXT("ULOSVisionSubsystem::RegisterProvider >>Invalid provider"));
		return false;
	}
	if (InVisionChannel==EVisionChannel::None)
	{
		UE_LOG(LOSVisionSubsystem, Error,
			TEXT("ULOSVisionSubsystem::RegisterProvider >>VisionChannel not settled"));
		return false;
	}

	// Get or create the channel entry
	FRegisteredProviders& ChannelEntry = VisionMap.FindOrAdd(InVisionChannel);
	if (ChannelEntry.RegisteredList.Contains(Provider))
	{
		UE_LOG(LOSVisionSubsystem, Warning,
			TEXT("ULOSVisionSubsystem::RegisterProvider >> Already registered provider[%s] in channel:%d"),
			*Provider->GetOwner()->GetName(),
			InVisionChannel);
		return false;
	}

	// Add to the list
	ChannelEntry.RegisteredList.Add(Provider);
	UE_LOG(LOSVisionSubsystem, Log,
		TEXT("ULOSVisionSubsystem::RegisterProvider >> Provider[%s] registered. channel:%d"),
		*Provider->GetOwner()->GetName(),
		InVisionChannel);

	return true;
}

void ULOSVisionSubsystem::UnregisterProvider(ULineOfSightComponent* Provider, EVisionChannel InVisionChannel)
{
	if (!Provider)
	{
		UE_LOG(LOSVisionSubsystem, Error, TEXT("ULOSVisionSubsystem::UnregisterProvider >> Invalid provider"));
		return;
	}

	// Try to find channel
	if (FRegisteredProviders* ChannelEntry = VisionMap.Find(InVisionChannel))
	{
		// Remove provider if it exists
		if (ChannelEntry->RegisteredList.Remove(Provider) > 0)
		{
			UE_LOG(LOSVisionSubsystem, Log,
				TEXT("ULOSVisionSubsystem::UnregisterProvider >> Provider[%s] unregistered from channel:%d"),
				*Provider->GetOwner()->GetName(),
				InVisionChannel);
			return; // Success, stop
		}
	}

	// Provider not found in channel
	UE_LOG(LOSVisionSubsystem, Warning,
		TEXT("ULOSVisionSubsystem::UnregisterProvider >> Could not find Provider[%s] in channel:%d"),
		*Provider->GetOwner()->GetName(),
		InVisionChannel);
}

TArray<ULineOfSightComponent*> ULOSVisionSubsystem::GetProvidersForTeam(EVisionChannel TeamChannel) const
{
	TArray<ULineOfSightComponent*> OutProviders;

	// Add providers from the requested team channel
	if (const FRegisteredProviders* TeamEntry = VisionMap.Find(TeamChannel))
	{
		OutProviders.Append(TeamEntry->RegisteredList);
	}
	else
	{
		UE_LOG(LOSVisionSubsystem, Error,
			TEXT("ULOSVisionSubsystem::GetProvidersForTeam >> No providers found for team channel:%d"),
			TeamChannel);
	}

	// Add providers from shared vision channel
	if (const FRegisteredProviders* SharedEntry = VisionMap.Find(EVisionChannel::SharedVision))
	{
		OutProviders.Append(SharedEntry->RegisteredList);
	}
	else
	{
		UE_LOG(LOSVisionSubsystem, Error,
			TEXT("ULOSVisionSubsystem::GetProvidersForTeam >> No shared vision providers found"));
	}

	return OutProviders;
}