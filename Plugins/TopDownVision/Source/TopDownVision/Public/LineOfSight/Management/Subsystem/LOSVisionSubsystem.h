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

    // Key: team ID.  Value: set of observers on that team currently seeing the target.
    TMap<uint8, TSet<TWeakObjectPtr<AActor>>> ObserversByTeam;
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
    TArray<UVision_VisualComp*> GetAllProviders() const;

    /** Returns only providers whose team the local player is eligible to see.
    *  Respects TeamChannel, bAllReveal, and AlwaysVisible.
    *  Falls back to GetAllProviders() if VisionPS is not yet ready. */
    TArray<UVision_VisualComp*> GetProvidersVisibleToLocalPlayer() const;

    // --- Visibility reporting --- //
    void ReportTargetVisibility(
        AActor* Observer,
        EVisionChannel ObserverTeam,
        AActor* Target,
        bool bVisible);

    // --- Local player lookup (shared with GameStateComp) --- //
    static UVisionPlayerStateComp* GetLocalVisionPS(UWorld* World);

private:
    void HandleProviderRegistered(UVision_VisualComp* NewProvider, EVisionChannel Channel);
    UVisionGameStateComp* GetVisionGameStateComp() const;

public:
    UPROPERTY()
    TMap<EVisionChannel, FRegisteredProviders> VisionMap;

private:
    
    TMap<AActor*, FTargetVisibilityVotes> VisibilityVotes;
};