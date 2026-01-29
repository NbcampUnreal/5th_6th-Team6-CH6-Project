// Fill out your copyright notice in the Description page of Project Settings.


#include "LineOfSight/VisionSubsystem.h"
#include "LineOfSight/LineOfSightComponent.h"


//LOG
DEFINE_LOG_CATEGORY(VisionSubsystem);

void UVisionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	UE_LOG(VisionSubsystem, Log,
		TEXT("UVisionSubsystem::Initialize >> VisionSubsystem initialized"));
}

void UVisionSubsystem::Deinitialize()
{
	UE_LOG(VisionSubsystem, Log,
		TEXT("UVisionSubsystem::Deinitialize >> VisionSubsystem Deinitialize"));
	
	Super::Deinitialize();
}

bool UVisionSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// no editor previews. only in Game World
	if (const UWorld* World = Cast<UWorld>(Outer))
	{
		return World->IsGameWorld();
	}

	//could not bring any valid world, so this is also false
	return false;
}

bool UVisionSubsystem::RegisterProvider(ULineOfSightComponent* Provider, EVisionChannel InVisionChannel)
{
	if (!Provider)
	{
		UE_LOG(VisionSubsystem, Error,
			TEXT("UVisionSubsystem::RegisterProvider >>Invalid provider"));
		return false;
	}
	if (InVisionChannel==EVisionChannel::None)
	{
		UE_LOG(VisionSubsystem, Error,
			TEXT("UVisionSubsystem::RegisterProvider >>VisionChannel not settled"));
		return false;
	}

	// Get or create the channel entry
	FRegisteredProviders& ChannelEntry = VisionMap.FindOrAdd(InVisionChannel);
	if (ChannelEntry.RegisteredList.Contains(Provider))
	{
		UE_LOG(VisionSubsystem, Warning,
			TEXT("UVisionSubsystem::RegisterProvider >> Already registered provider[%s] in channel:%d"),
			*Provider->GetOwner()->GetName(),
			InVisionChannel);
		return false;
	}

	// Add to the list
	ChannelEntry.RegisteredList.Add(Provider);
	UE_LOG(VisionSubsystem, Log,
		TEXT("UVisionSubsystem::RegisterProvider >> Provider[%s] registered. channel:%d"),
		*Provider->GetOwner()->GetName(),
		InVisionChannel);

	return true;
}

void UVisionSubsystem::UnregisterProvider(ULineOfSightComponent* Provider, EVisionChannel InVisionChannel)
{
	if (!Provider)
	{
		UE_LOG(VisionSubsystem, Error, TEXT("UVisionSubsystem::UnregisterProvider >> Invalid provider"));
		return;
	}

	// Try to find channel
	if (FRegisteredProviders* ChannelEntry = VisionMap.Find(InVisionChannel))
	{
		// Remove provider if it exists
		if (ChannelEntry->RegisteredList.Remove(Provider) > 0)
		{
			UE_LOG(VisionSubsystem, Log,
				TEXT("UVisionSubsystem::UnregisterProvider >> Provider[%s] unregistered from channel:%d"),
				*Provider->GetOwner()->GetName(),
				InVisionChannel);
			return; // Success, stop
		}
	}

	// Provider not found in channel
	UE_LOG(VisionSubsystem, Warning,
		TEXT("UVisionSubsystem::UnregisterProvider >> Could not find Provider[%s] in channel:%d"),
		*Provider->GetOwner()->GetName(),
		InVisionChannel);
}

TArray<ULineOfSightComponent*> UVisionSubsystem::GetProvidersForTeam(EVisionChannel TeamChannel) const
{
	TArray<ULineOfSightComponent*> OutProviders;

	// Add providers from the requested team channel
	if (const FRegisteredProviders* TeamEntry = VisionMap.Find(TeamChannel))
	{
		OutProviders.Append(TeamEntry->RegisteredList);
	}
	else
	{
		UE_LOG(VisionSubsystem, Error,
			TEXT("UVisionSubsystem::GetProvidersForTeam >> No providers found for team channel:%d"),
			TeamChannel);
	}

	// Add providers from shared vision channel
	if (const FRegisteredProviders* SharedEntry = VisionMap.Find(EVisionChannel::SharedVision))
	{
		OutProviders.Append(SharedEntry->RegisteredList);
	}
	else
	{
		UE_LOG(VisionSubsystem, Error,
			TEXT("UVisionSubsystem::GetProvidersForTeam >> No shared vision providers found"));
	}

	return OutProviders;
}