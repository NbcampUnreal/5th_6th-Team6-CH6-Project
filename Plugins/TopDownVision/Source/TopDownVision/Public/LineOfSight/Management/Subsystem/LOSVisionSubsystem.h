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

    // Key: Observer actor | Value: true = currently reporting visible
    UPROPERTY()
    TMap<AActor*, bool> VotesByObserver;

    // Returns true if any observer belonging to the given team is reporting visible
    bool HasAnyVisibleVoteForTeam(EVisionChannel Team) const;
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
    TArray<UVision_VisualComp*> GeAllProviders() const;

    // --- Visibility reporting --- //
    // Called server-side directly (standalone or via RPC)
    void ReportTargetVisibility(AActor* Observer, EVisionChannel ObserverTeam, AActor* Target, bool bVisible);

    // Called from client — routes through PlayerState RPC in multiplayer, direct in standalone
    void ReportTargetVisibilityFromClient(AActor* Observer, EVisionChannel ObserverTeam, AActor* Target, bool bVisible);

    // --- Debug --- //
    UFUNCTION(BlueprintCallable, Category="LineOfSight|Debug")
    void PrintVisionStatus() const;

    // --- Local player lookup --- //
    static UVisionPlayerStateComp* GetLocalVisionPS(UWorld* World);

private:
    void HandleProviderRegistered(UVision_VisualComp* NewProvider, EVisionChannel Channel);
    UVisionGameStateComp* GetVisionGameStateComp() const;

public:
    UPROPERTY()
    TMap<EVisionChannel, FRegisteredProviders> VisionMap;

private:
    UPROPERTY()
    TMap<AActor*, FTargetVisibilityVotes> VisibilityVotes;
};