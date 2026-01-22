// Fill out your copyright notice in the Description page of Project Settings.


#include "ER_OutGameMode.h"
#include "ER_PlayerState.h"



void AER_OutGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (!HasAuthority() || !NewPlayer) return;

    if (APlayerState* PS = NewPlayer->GetPlayerState<APlayerState>())
    {
        if (AER_PlayerState* ERPS = Cast<AER_PlayerState>(PS))
        {
            //ERPS->SetPlayerName(ERPS->GetPlayerStateName());
            UE_LOG(LogTemp, Warning, TEXT("%s"), *ERPS->GetPlayerName());
            //ERPS->ForceNetUpdate();
        }

    }
}

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
