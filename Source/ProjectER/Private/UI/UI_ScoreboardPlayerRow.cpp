// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/UI_ScoreboardPlayerRow.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"

#include "Kismet/GameplayStatics.h" // gamestate용
#include "GameModeBase/State/ER_GameState.h" // gamestate
#include "GameModeBase/State/ER_PlayerState.h"

void UUI_ScoreboardPlayerRow::Init(AER_PlayerState* PlayerState)
{
	UpdatePlayerName(FText::FromString(PlayerState->GetPlayerName()));
}

void UUI_ScoreboardPlayerRow::UpdatePlayerName(FText PlayerName)
{
	if(txtPlayerName)
	{
		txtPlayerName->SetText(PlayerName);
	}
}

void UUI_ScoreboardPlayerRow::UpdatePlayerLV(float PlayerLV)
{
	if(txtPlayerLV)
	{
		txtPlayerLV->SetText(FText::AsNumber(FMath::RoundToInt(PlayerLV)));
	}
}

void UUI_ScoreboardPlayerRow::UpdatePlayerKill(float PlayerKill)
{
	if(txtPlayerKill)
	{
		txtPlayerKill->SetText(FText::AsNumber(FMath::RoundToInt(PlayerKill)));
	}
}

void UUI_ScoreboardPlayerRow::UpdatePlayerDeath(float PlayerDeath)
{
	if(txtPlayerDeath)
	{
		txtPlayerDeath->SetText(FText::AsNumber(FMath::RoundToInt(PlayerDeath)));
	}
}

void UUI_ScoreboardPlayerRow::UpdatePlayerAssist(float PlayerAssist)
{
	if(txtPlayerAssist)
	{
		txtPlayerAssist->SetText(FText::AsNumber(FMath::RoundToInt(PlayerAssist)));
	}
}

void UUI_ScoreboardPlayerRow::UpdatePlayerHP(float CurrentHP, float MaxHP)
{
	if (PB_HP)
	{
		float HealthPercent = CurrentHP / MaxHP;
		PB_HP->SetPercent(HealthPercent);
	}
	if (txtHP)
	{
		txtHP->SetText(FText::Format(NSLOCTEXT("Scoreboard", "HPFormat", "{0} / {1}"), FText::AsNumber(FMath::RoundToInt(CurrentHP)), FText::AsNumber(FMath::RoundToInt(MaxHP))));
	}
}
