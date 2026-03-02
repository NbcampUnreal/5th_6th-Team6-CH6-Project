// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LOSVisionSubsystem.generated.h"

/**
 *  This is for the LOS vision provider comp.
 */


enum class EVisionChannel : uint8;
//forward declare
class UVisionTargetComp;//changed source of the update


USTRUCT()
struct FRegisteredProviders
{
	GENERATED_BODY()
	
	UPROPERTY()
	TArray<UVisionTargetComp*> RegisteredList;
};

//Log

TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(LOSVisionSubsystem, Log, All);

UCLASS()
class TOPDOWNVISION_API ULOSVisionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()


public:	

	//Registration
	UFUNCTION(BlueprintCallable, Category="LineOfSight")
	bool RegisterProvider(UVisionTargetComp* Provider, EVisionChannel InVisionChannel);
	UFUNCTION(BlueprintCallable, Category="LineOfSight")
	void UnregisterProvider(UVisionTargetComp* Provider, EVisionChannel InVisionChannel);

	// getter of same team+shared vision
	TArray<UVisionTargetComp*> GetProvidersForTeam(EVisionChannel TeamChannel) const;

	// Registered actor-local LOS providers
	UPROPERTY()
	TMap<EVisionChannel, FRegisteredProviders> VisionMap;
};
