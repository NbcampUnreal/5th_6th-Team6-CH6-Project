// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */

UENUM()
enum E2DShapeType : uint8
{
	None       = 255 UMETA(DisplayName = "None"),
    
	Circle     = 0   UMETA(DisplayName = "Circle"),
	Square     = 1   UMETA(DisplayName = "Square"),
	FreePoints = 2   UMETA(DisplayName = "FreePoints"),
};

