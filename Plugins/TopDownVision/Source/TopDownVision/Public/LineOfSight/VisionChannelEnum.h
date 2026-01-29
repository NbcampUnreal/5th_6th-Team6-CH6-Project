// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

UENUM()
enum class EVisionChannel : uint8
{
	None= 255 UMETA(DisplayName = "None"), // special: no channel
	
	SharedVision = 0 UMETA(DisplayName = "SharedVision"),
	TeamA = 1 UMETA(DisplayName = "TeamA"),
	TeamB = 2 UMETA(DisplayName = "TeamB"),
	TeamC = 3 UMETA(DisplayName = "TeamC"),
};