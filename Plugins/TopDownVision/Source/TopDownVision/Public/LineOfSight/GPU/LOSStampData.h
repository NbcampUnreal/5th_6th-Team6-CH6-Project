// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

//Matching struct for GPU StructuredBuffer
struct FLOSStampData
{
	FVector4f CenterRadiusStrength; 
	// xy = CenterUV
	// z  = Radius
	// w  = Strength

	uint32 ChannelBitMask; // VISIBILITY mask, NOT index
	uint32 Padding[3];     // align to 32 bytes
};
// HLSL aligns in 16-byte boundaries ->16*2
static_assert(sizeof(FLOSStampData) == 32, "FLOSStampData must be 32 bytes");
static_assert(alignof(FLOSStampData) == 16, "FLOSStampData must be 16-byte aligned");

/*
	FVector2f CenterUV; 8bit
	float Radius;// 4bit
	float Strength;// 4bit
	uint32 Channel;/ 4bit

	uint32*3 Padding// 4*3
	
	total=32 bit
 */