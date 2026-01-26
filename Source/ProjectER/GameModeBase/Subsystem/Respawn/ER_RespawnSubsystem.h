// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ER_RespawnSubsystem.generated.h"

class AER_PlayerState;
class AER_GameState;

UCLASS()
class PROJECTER_API UER_RespawnSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void HandlePlayerDeath(AER_PlayerState& PS, AER_GameState& GS);
	
};
