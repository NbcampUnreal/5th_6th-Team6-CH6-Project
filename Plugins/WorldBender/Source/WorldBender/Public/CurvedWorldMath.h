// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
// this is only for static function, shared by other class to use it for placement correction, curve modification and etc

static void WorldBenderMath(
	//updated values
	const FVector& WorldLocation,
	const FVector& ForwardVector,
	const FVector& RightVector,
	
	//intensity
	const float& ForwardWarp,
	const float& RightWarp,
	const float& WarpWeight,
	//out
	FVector& OutLocation)
{
	// math here
}