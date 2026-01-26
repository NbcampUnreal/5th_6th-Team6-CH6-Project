// Fill out your copyright notice in the Description page of Project Settings.


#include "ER_RespawnSubsystem.h"
#include "GameModeBase/State/ER_PlayerState.h"
#include "GameModeBase/State/ER_GameState.h"


void UER_RespawnSubsystem::HandlePlayerDeath(AER_PlayerState& PS, AER_GameState& GS)
{
	if (!GS.HasAuthority())
		return;

	if (PS.bIsDead)
		return;

	PS.bIsDead = true;
	UE_LOG(LogTemp, Log, TEXT("bIsDead"));
	// 이후 처리 로직 추가
}
