// Fill out your copyright notice in the Description page of Project Settings.


#include "ER_OutGameMode.h"

void AER_OutGameMode::StartGame()
{
	if (!HasAuthority())
		return;

	GetWorld()->ServerTravel("/Game/Level/BasicMap", true);
}

void AER_OutGameMode::EndGame()
{
	if (!HasAuthority())
		return;

	GetWorld()->ServerTravel("/Game/Level/Level_Lobby", true);
}
