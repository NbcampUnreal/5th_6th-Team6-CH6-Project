// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

UENUM()
enum class EVisionChannel : uint8
{
	None= 255 UMETA(DisplayName = "None"), // special: no channel
	
	SharedVision = 3 UMETA(DisplayName = "SharedVision"),// same as netural
	
	TeamA = 0 UMETA(DisplayName = "TeamA"),
	TeamB = 1 UMETA(DisplayName = "TeamB"),
	TeamC = 2 UMETA(DisplayName = "TeamC"),

	//changed 
};

UENUM()
enum class EObstacleType : uint8
{
	None= 255 UMETA(DisplayName = "None"), // invalid type

	ShadowCastable = 0 UMETA(DisplayName = "ShadowCastable"),
	None_ShadowCastable = 1 UMETA(DisplayName = "NoneShadowCastable"),
	
	// could use like translucent in setting?
};

