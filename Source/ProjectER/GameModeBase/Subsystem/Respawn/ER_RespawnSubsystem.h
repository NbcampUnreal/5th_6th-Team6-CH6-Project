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
	void HandlePlayerDeath(AER_PlayerState& PS, AER_GameState& GS, AER_PlayerState* KillerPS, const TArray<APlayerState*>& Assists);
	bool EvaluateTeamElimination(AER_PlayerState& PS, AER_GameState& GS);

	// 패배 처리
	void SetTeamLose(AER_GameState& GS, int32 TeamIdx);

	// 승리 처리
	void SetTeamWin(AER_GameState& GS, int32 TeamIdx);

	//리스폰 타이머
	void StartRespawnTimer(AER_PlayerState& PS, AER_GameState& GS);
	void StopResapwnTimer(AER_GameState& GS, int32 TeamIdx);
	
	//리스폰 처리
	void RespawnPlayer();

	// 마지막 팀인지 확인하기
	int32 CheckIsLastTeam(AER_GameState& GS);

	void InitializeRespawnMap(AER_GameState& GS);


private:
	TMap<int32, FTimerHandle> RespawnMap;
};
