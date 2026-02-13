// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(LOSVision, Log, All);
TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(LOSTrace, Log, All);

TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(LOSWorldBaker, Log, All);// for editor function


//Server/Client Name Helper fun__function_ignore_lock_checking(
//)
namespace TopDownVisionDebug
{
	TOPDOWNVISION_API FString GetClientDebugName(const UObject* WorldContextObject);
	TOPDOWNVISION_API FString GetNetModeString(const UWorld* World);
}