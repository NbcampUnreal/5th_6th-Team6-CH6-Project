// Fill out your copyright notice in the Description page of Project Settings.


#include "ER_OutGamePlayerController.h"
#include "ER_OutGameMode.h"

void AER_OutGamePlayerController::ConnectToDedicatedServer(const FString& Ip, int32 Port)
{
	if (!IsLocalController())
		return;

	const FString Address = FString::Printf(TEXT("%s:%d"), *Ip, Port);

	UE_LOG(LogTemp, Log, TEXT("[PC] Connecting to server: %s"), *Address);

	ClientTravel(Address, TRAVEL_Absolute);
}


void AER_OutGamePlayerController::Server_StartGame_Implementation()
{
	auto OutGameMode = Cast<AER_OutGameMode>(GetWorld()->GetAuthGameMode());
	OutGameMode->StartGame();
}

void AER_OutGamePlayerController::Server_TEMP_OutGame_Implementation()
{
	auto OutGameMode = Cast<AER_OutGameMode>(GetWorld()->GetAuthGameMode());
	OutGameMode->EndGame();
}