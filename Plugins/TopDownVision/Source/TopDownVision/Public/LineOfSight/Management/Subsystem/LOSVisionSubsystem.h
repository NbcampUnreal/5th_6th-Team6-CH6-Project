#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LineOfSight/VisionData.h"
#include "LOSVisionSubsystem.generated.h"

class UVision_VisualComp;
class UVisionGameStateComp;
class UVisionPlayerStateComp;

USTRUCT()
struct FRegisteredProviders
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<UVision_VisualComp*> RegisteredList;
};

USTRUCT()
struct FTargetVisibilityVotes
{
	GENERATED_BODY()

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

	TArray<UVision_VisualComp*> GeAlltProviders() const;

	// --- Visibility reporting --- //
	void ReportTargetVisibility(
		AActor* Observer,
		EVisionChannel ObserverTeam,
		AActor* Target,
		bool bVisible);

	// --- Local player lookup (used by GameStateComp) --- //
	static UVisionPlayerStateComp* GetLocalVisionPS(UWorld* World);

private:
	// Handles reveal logic when a new provider joins — owns what was OnProviderRegistered
	void HandleProviderRegistered(UVision_VisualComp* NewProvider, EVisionChannel Channel);

	UVisionGameStateComp* GetVisionGameStateComp() const;

public:
	UPROPERTY()
	TMap<EVisionChannel, FRegisteredProviders> VisionMap;

private:
	UPROPERTY()
	TMap<AActor*, FTargetVisibilityVotes> VisibilityVotes;
};