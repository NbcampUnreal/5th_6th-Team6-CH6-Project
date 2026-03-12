// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */


//LOG DECLARE
PROJECTER_API DECLARE_LOG_CATEGORY_EXTERN(LevelAreaGraphManagement, Log, All);


namespace DebugLogHelper
{
	PROJECTER_API FString GetClientDebugName(const UObject* WorldContextObject);
	PROJECTER_API FString GetNetModeString(const UWorld* World);
}


