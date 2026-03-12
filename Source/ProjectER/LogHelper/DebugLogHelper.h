// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
namespace DebugLogHelper
{
	PROJECTER_API FString GetClientDebugName(const UObject* WorldContextObject);
	PROJECTER_API FString GetNetModeString(const UWorld* World);
}



PROJECTER_API DECLARE_LOG_CATEGORY_EXTERN(LevelAreaGraphManagement, Log, All);