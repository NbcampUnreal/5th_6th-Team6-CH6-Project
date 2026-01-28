// Fill out your copyright notice in the Description page of Project Settings.


#include "ER_RespawnSubsystem.h"
#include "GameModeBase/State/ER_PlayerState.h"
#include "GameModeBase/State/ER_GameState.h"
#include "GameModeBase/ER_OutGamePlayerController.h"


void UER_RespawnSubsystem::HandlePlayerDeath(AER_PlayerState& PS, AER_GameState& GS)
{
	if (!GS.HasAuthority())
		return;

	UE_LOG(LogTemp, Warning, TEXT("[RSS] : Start HandlePlayerDeath"));


	if (PS.bIsDead)
		return;

	PS.bIsDead = true;
	PS.ForceNetUpdate();
	PS.FlushNetDormancy();
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

void UER_RespawnSubsystem::SetTeamLose(AER_GameState& GS, int32 TeamIdx)
{
	for (auto& player : GS.GetTeamArray(TeamIdx))
	{
		AER_OutGamePlayerController* PC = Cast<AER_OutGamePlayerController>(player->GetOwner());

		PC->Client_SetLose();
	}
}

void UER_RespawnSubsystem::SetTeamWin(AER_GameState& GS, int32 TeamIdx)
{
	for (auto& player : GS.GetTeamArray(TeamIdx))
	{
		AER_OutGamePlayerController* PC = Cast<AER_OutGamePlayerController>(player->GetOwner());

		PC->Client_SetWin();
	}
}

int32 UER_RespawnSubsystem::CheckIsLastTeam(AER_GameState& GS)
{
	if (!GS.HasAuthority())
		return -1;

	UE_LOG(LogTemp, Warning, TEXT("[RSS] : Start CheckIsLastTeam"));

	int32 LastTeamIdx = GS.GetLastTeamIdx();

	// -1 이면 실패 혹은 마지막 팀이 아님
	return LastTeamIdx;

}




