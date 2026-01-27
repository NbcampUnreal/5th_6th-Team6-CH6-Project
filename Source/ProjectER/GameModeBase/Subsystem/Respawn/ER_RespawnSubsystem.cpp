// Fill out your copyright notice in the Description page of Project Settings.


#include "ER_RespawnSubsystem.h"
#include "GameModeBase/State/ER_PlayerState.h"
#include "GameModeBase/State/ER_GameState.h"


void UER_RespawnSubsystem::HandlePlayerDeath(AER_PlayerState& PS, AER_GameState& GS)
{
	if (!GS.HasAuthority())
		return;

	UE_LOG(LogTemp, Warning, TEXT("[RSS] : Start HandlePlayerDeath"));


	if (PS.bIsDead)
		return;

	PS.bIsDead = true;
	UE_LOG(LogTemp, Warning, TEXT("PS.bIsDead = %s"), PS.bIsDead ? TEXT("True") : TEXT("False"));
}

bool UER_RespawnSubsystem::EvaluateTeamElimination(AER_PlayerState& PS, AER_GameState& GS)
{
	if (!GS.HasAuthority())
		return false;

	UE_LOG(LogTemp, Warning, TEXT("[RSS] : Start EvaluateTeamElimination"));


	int32 TeamIdx = static_cast<int32>(PS.Team);

	return GS.GetTeamEliminate(TeamIdx);

}

void UER_RespawnSubsystem::ShowLoseUI(AER_PlayerState& PS)
{
	if (!PS.HasAuthority())
		return;

	UE_LOG(LogTemp, Warning, TEXT("[RSS] : Start ShowLoseUI"));

	if (PS.bIsLose)
		return;

	PS.bIsLose = true;
}

void UER_RespawnSubsystem::ShowWinUI(AER_PlayerState& PS)
{
	if (!PS.HasAuthority())
		return;

	UE_LOG(LogTemp, Warning, TEXT("[RSS] : Start ShowWinUI"));

	if (PS.bIsWin)
		return;

	PS.bIsWin = true;
}

void UER_RespawnSubsystem::CheckIsLastTeam(AER_GameState& GS)
{
	if (!GS.HasAuthority())
		return;

	UE_LOG(LogTemp, Warning, TEXT("[RSS] : Start CheckIsLastTeam"));

	int32 LastTeamIdx = GS.GetLastTeamIdx();

	if (LastTeamIdx == -1)
		return;

	GS.SetTeamWin(LastTeamIdx);
}

