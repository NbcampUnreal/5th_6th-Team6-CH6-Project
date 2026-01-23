// Fill out your copyright notice in the Description page of Project Settings.


#include "ER_OutGameMode.h"
#include "ER_PlayerState.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"

FString AER_OutGameMode::InitNewPlayer(APlayerController* NewPlayerController,
    const FUniqueNetIdRepl& UniqueId,
    const FString& Options,
    const FString& Portal)
{
    Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);

    // URL에서 닉네임 파싱
    FString UserName = UGameplayStatics::ParseOption(Options, TEXT("UserName"));

    if (!UserName.IsEmpty())
    {
        // PlayerState에 닉네임 설정
        if (APlayerState* PS = NewPlayerController->GetPlayerState<APlayerState>())
        {
            
            if (AER_PlayerState* ERPS = Cast<AER_PlayerState>(PS))
            {
                ERPS->SetPlayerName(UserName);
                ERPS->SetPlayerStateName(UserName);

                UE_LOG(LogTemp, Warning, TEXT("InitNewPlayer : %s"), *ERPS->GetPlayerName());
                return TEXT("");
            }
        }
    }
    return TEXT("");
}

void AER_OutGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (!HasAuthority() || !NewPlayer) return;

    if (APlayerState* PS = NewPlayer->GetPlayerState<APlayerState>())
    {
        if (AER_PlayerState* ERPS = Cast<AER_PlayerState>(PS))
        {

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
