#pragma once

#include "CoreMinimal.h"
#include "LevelAreaData.generated.h"

UENUM(BlueprintType)
enum class EAreaHazardState : uint8
{
	None         = 255  UMETA(DisplayName = "None"),
	Safe         = 0    UMETA(DisplayName = "Safe"),
	Hazard       = 1    UMETA(DisplayName = "Hazard"),
	InstantDeath = 2    UMETA(DisplayName = "InstantDeath"),
};

//Put Delegate in here!!!!

// Used by ALevelAreaActor — broadcasts current state only
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAreaHazardStateChanged, EAreaHazardState, NewState);

// Used by ULevelAreaTrackerComponent — broadcasts transition (old → new)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAreaHazardStateTransition, EAreaHazardState, OldState, EAreaHazardState, NewState);

// Used by ULevelAreaTrackerComponent — broadcasts node change
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAreaNodeChanged, int32, OldNodeID, int32, NewNodeID);