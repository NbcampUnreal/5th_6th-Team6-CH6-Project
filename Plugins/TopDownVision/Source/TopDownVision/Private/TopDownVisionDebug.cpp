// Fill out your copyright notice in the Description page of Project Settings.


//name log
#include "TopDownVisionDebug.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"


DEFINE_LOG_CATEGORY(LOSVision);
DEFINE_LOG_CATEGORY(LOSTrace);
DEFINE_LOG_CATEGORY(LOSWorldBaker);

namespace TopDownVisionDebug
{
	FString GetNetModeString(const UWorld* World)
	{
		if (!World)
			return TEXT("NoWorld");

		switch (World->GetNetMode())
		{
		case NM_Client:          return TEXT("Client");
		case NM_ListenServer:    return TEXT("ListenServer");
		case NM_Standalone:      return TEXT("Standalone");
		case NM_DedicatedServer: return TEXT("DedicatedServer");
		default:                 return TEXT("UnknownNetMode");
		}
	}

	FString GetClientDebugName(const UObject* WorldContextObject)
	{
		if (!WorldContextObject)
			return TEXT("[NoContext]");

		const UWorld* World = WorldContextObject->GetWorld();
		const FString NetModeStr = GetNetModeString(World);

		FString PlayerStr = TEXT("NoPlayer");

		if (World)
		{
			if (APlayerController* PC = World->GetFirstPlayerController())
			{
				if (APlayerState* PS = PC->GetPlayerState<APlayerState>())
				{
					PlayerStr = FString::Printf(
						TEXT("%s(%d)"),
						*PS->GetPlayerName(),
						PS->GetPlayerId());
				}
			}
		}

		return FString::Printf(TEXT("[%s | %s]"), *NetModeStr, *PlayerStr);
	}
}