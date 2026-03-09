#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class EVisionChannel : uint8
{
	None         = 255 UMETA(DisplayName = "None"),
	SharedVision = 3   UMETA(DisplayName = "SharedVision"),
	TeamA        = 0   UMETA(DisplayName = "TeamA"),
	TeamB        = 1   UMETA(DisplayName = "TeamB"),
	TeamC        = 2   UMETA(DisplayName = "TeamC"),
};

UENUM(BlueprintType)
enum class EObstacleType : uint8
{
	None                = 255 UMETA(DisplayName = "None"),
	ShadowCastable      = 0   UMETA(DisplayName = "ShadowCastable"),
	None_ShadowCastable = 1   UMETA(DisplayName = "NoneShadowCastable"),
};