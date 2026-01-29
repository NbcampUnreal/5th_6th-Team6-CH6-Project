#include "ER_OutGamePlayerController.h"
#include "GameModeBase/State/ER_PlayerState.h"
#include "GameMode/ER_OutGameMode.h"
#include "GameMode/ER_InGameMode.h"
#include "Blueprint/UserWidget.h"


AER_OutGamePlayerController::AER_OutGamePlayerController()
{

}

void AER_OutGamePlayerController::BeginPlay()
{
	Super::BeginPlay();
}


void AER_OutGamePlayerController::ConnectToDedicatedServer(const FString& Ip, int32 Port, const FString& PlayerName)
{
	if (!IsLocalController())
		return;

	const FString Address = FString::Printf(TEXT("%s:%d?PlayerName=%s"), *Ip, Port, *PlayerName);

	UE_LOG(LogTemp, Log, TEXT("[PC] Connecting to server: %s"), *Address);

	ClientTravel(Address, TRAVEL_Absolute);
}




void AER_OutGamePlayerController::ShowWinUI()
{
	if (!WinUIClass)
		return;

	if (IsValid(WinUIInstance))
		return;

	UE_LOG(LogTemp, Log, TEXT("[PC] : ShowWinUI"));
	WinUIInstance = CreateWidget<UUserWidget>(this, WinUIClass);
	WinUIInstance->AddToViewport();
}

void AER_OutGamePlayerController::ShowLoseUI()
{
	if (!LoseUIClass)
		return;

	if (IsValid(LoseUIInstance))
		return;

	UE_LOG(LogTemp, Log, TEXT("[PC] : ShowLoseUI"));
	LoseUIInstance = CreateWidget<UUserWidget>(this, LoseUIClass);
	LoseUIInstance->AddToViewport();
}

void AER_OutGamePlayerController::ShowRespawnTimerUI()
{
	if (!RespawnUIClass)
		return;

	if (IsValid(RespawnUIInstance))
		return;

	UE_LOG(LogTemp, Log, TEXT("[PC] : ShowRespawnUI"));
	RespawnUIInstance = CreateWidget<UUserWidget>(this, RespawnUIClass);
	RespawnUIInstance->AddToViewport();
}

void AER_OutGamePlayerController::HideRespawnTimerUI()
{
	if (IsValid(RespawnUIInstance))
	{
		RespawnUIInstance->RemoveFromParent();
		RespawnUIInstance = nullptr;
	}
}

void AER_OutGamePlayerController::Server_StartGame_Implementation()
{
	auto OutGameMode = Cast<AER_OutGameMode>(GetWorld()->GetAuthGameMode());
	OutGameMode->StartGame();
}

void AER_OutGamePlayerController::Server_DisConnectServer_Implementation()
{
	auto InGameMode = Cast<AER_InGameMode>(GetWorld()->GetAuthGameMode());

	InGameMode->DisConnectClient(this);
}

void AER_OutGamePlayerController::Client_SetLose_Implementation()
{
	AER_PlayerState* PS = GetPlayerState<AER_PlayerState>();
	PS->bIsLose = true;
	ShowLoseUI();
}

void AER_OutGamePlayerController::Client_SetWin_Implementation()
{
	AER_PlayerState* PS = GetPlayerState<AER_PlayerState>();
	PS->bIsWin = true;
	ShowWinUI();
}

void AER_OutGamePlayerController::Client_SetDead_Implementation()
{
	AER_PlayerState* PS = GetPlayerState<AER_PlayerState>();
	PS->bIsDead = true;
}

void AER_OutGamePlayerController::Client_StartRespawnTimer_Implementation()
{
	ShowRespawnTimerUI();
}

void AER_OutGamePlayerController::Client_StopRespawnTimer_Implementation()
{
	HideRespawnTimerUI();
}