#include "ER_OutGamePlayerController.h"
#include "GameModeBase/State/ER_PlayerState.h"
#include "GameMode/ER_OutGameMode.h"
#include "Blueprint/UserWidget.h"

// 인 게임 컨트롤러로 이식 필요
void AER_OutGamePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (AER_PlayerState* PS = GetPlayerState<AER_PlayerState>())
	{
		PS->OnWinChanged.AddUObject(this, &AER_OutGamePlayerController::HandleLoseChanged);
		PS->OnWinChanged.AddUObject(this, &AER_OutGamePlayerController::HandleWinChanged);
	}
}

// 인 게임 컨트롤러로 이식 필요
void AER_OutGamePlayerController::HandleLoseChanged(bool bLose)
{
	if (!IsLocalController()) return;

	if (bLose)
	{
		// 탈락 UI 띄우기, 입력 제한, 커서 ON
		if (!LoseUIClass)
			return;

		UE_LOG(LogTemp, Log, TEXT("[PC] : HandleLoseChanged"));
		LoseUIInstance = CreateWidget<UUserWidget>(this, LoseUIClass);
	}
	else
	{
		
	}
}

// 인 게임 컨트롤러로 이식 필요
void AER_OutGamePlayerController::HandleWinChanged(bool bWin)
{
	if (!IsLocalController()) return;

	if (bWin)
	{
		// 승리 UI 띄우기, 입력 제한, 커서 ON
		if (!WinUIClass)
			return;

		UE_LOG(LogTemp, Log, TEXT("[PC] : HandleWinChanged"));
		WinUIInstance = CreateWidget<UUserWidget>(this, WinUIClass);
	}
	else
	{
		
	}
}

void AER_OutGamePlayerController::ConnectToDedicatedServer(const FString& Ip, int32 Port, const FString& PlayerName)
{
	if (!IsLocalController())
		return;

	const FString Address = FString::Printf(TEXT("%s:%d?PlayerName=%s"), *Ip, Port, *PlayerName);

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
