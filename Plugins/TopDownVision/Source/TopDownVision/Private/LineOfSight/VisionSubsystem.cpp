// Fill out your copyright notice in the Description page of Project Settings.


#include "LineOfSight/VisionSubsystem.h"


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

void UVisionSubsystem::RegisterProvider(ULineOfSightComponent* Provider)
{
}

void UVisionSubsystem::UnregisterProvider(ULineOfSightComponent* Provider)
{
}
