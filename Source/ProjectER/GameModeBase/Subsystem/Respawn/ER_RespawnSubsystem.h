// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ER_RespawnSubsystem.generated.h"

// 팀 전멸 판정, 부활 처리 담당

class AER_PlayerState;
class AER_GameState;

UCLASS()
class PROJECTER_API UER_RespawnSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void HandlePlayerDeath(AER_PlayerState& PS, AER_GameState& GS);
	bool EvaluateTeamElimination(AER_PlayerState& PS, AER_GameState& GS);
	
};
