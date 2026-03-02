// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LineOfSight/VisionData.h"
#include "LOSVisionSubsystem.generated.h"

class UVision_VisualComp;
class UVisionGameStateComp;

USTRUCT()
struct FRegisteredProviders
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<UVision_VisualComp*> RegisteredList;
};

// Tracks how many observers per team currently see a given target
USTRUCT()
struct FTargetVisibilityVotes
{
	GENERATED_BODY()

	// TeamID -> number of observers currently seeing this target
	TMap<uint8, int32> VotesByTeam;
};

TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(LOSVisionSubsystem, Log, All);

UCLASS()
class TOPDOWNVISION_API ULOSVisionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// --- Provider registration --- //
	UFUNCTION(BlueprintCallable, Category="LineOfSight")
	bool RegisterProvider(UVision_VisualComp* Provider, EVisionChannel InVisionChannel);

	UFUNCTION(BlueprintCallable, Category="LineOfSight")
	void UnregisterProvider(UVision_VisualComp* Provider, EVisionChannel InVisionChannel);

	// --- Provider access --- //
	TArray<UVision_VisualComp*> GetProvidersForTeam(EVisionChannel TeamChannel) const;

	// --- Visibility reporting --- //

	/** Called by Vision_EvaluatorComp every evaluation tick.
	 *  Aggregates votes and updates VisionGameStateComp when state changes. */
	void ReportTargetVisibility(
		AActor* Observer,
		EVisionChannel ObserverTeam,
		AActor* Target,
		bool bVisible);

private:
	UVisionGameStateComp* GetVisionGameStateComp() const;

public:
	UPROPERTY()
	TMap<EVisionChannel, FRegisteredProviders> VisionMap;

private:
	// Target -> vote counts per team
	TMap<AActor*, FTargetVisibilityVotes> VisibilityVotes;
};