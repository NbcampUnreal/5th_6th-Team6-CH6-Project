// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ER_PhaseSubsystem.generated.h"

class AER_GameState;

UCLASS()
class PROJECTER_API UER_PhaseSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	

public:
	void StartPhaseTimer(AER_GameState& GS, float Duration);
	void StartNoticeTimer(float Duration);
	void ClearPhaseTimer();

	void OnPhaseTimeUp();
	void OnNoticeTimeUp();

private:
	void OnPeriodicCheckTick();

private:
	FTimerHandle PhaseTimer;
	FTimerHandle NoticeTimer;
	FTimerHandle PeriodicCheckTimer;

	// 캐싱된 GameState. Subsystem이 소유권을 가지지 않으므로 TWeakObjectPtr 사용이 안전합니다.
	TWeakObjectPtr<class AER_GameState> CachedGameState;
};
