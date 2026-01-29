#include "GameModeBase/GameMode/ER_InGameMode.h"
#include "GameModeBase/State/ER_PlayerState.h"
#include "GameModeBase/State/ER_GameState.h"
#include "GameModeBase/Subsystem/Respawn/ER_RespawnSubsystem.h"

#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameSession.h"


void AER_InGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (UKismetSystemLibrary::IsDedicatedServer(GetWorld()))
	{
		return;
	}

	// 로컬에서만 실행
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (IsValid(PC))
	{
		FInputModeUIOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
	}
}

void AER_InGameMode::PostSeamlessTravel()
{
	Super::PostSeamlessTravel();

	PlayersInitialized = 0;

	UE_LOG(LogTemp, Warning, TEXT("[GM] PostSeamlessTravel - Expecting %d players"), ExpectedPlayers);
}

void AER_InGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	FString PlayerCountStr = UGameplayStatics::ParseOption(Options, TEXT("PlayerCount"));

	if (!PlayerCountStr.IsEmpty())
	{
		ExpectedPlayers = FCString::Atoi(*PlayerCountStr);
		UE_LOG(LogTemp, Warning, TEXT("[GM] InitGame - ExpectedPlayers from URL: %d"), ExpectedPlayers);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[GM] InitGame - No PlayerCount in URL Options!"));
	}
}

void AER_InGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	if (!NewPlayer)
		return;

	PlayersInitialized++;

	UE_LOG(LogTemp, Warning, TEXT("[GM] HandleStartingNewPlayer %d/%d"), PlayersInitialized, ExpectedPlayers);

	if (PlayersInitialized >= ExpectedPlayers)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GM] All players ready! Building team cache..."));

		AER_GameState* ERGS = GetGameState<AER_GameState>();
		if (ERGS)
		{
			// 다음 틱에 실행
			GetWorld()->GetTimerManager().SetTimerForNextTick([ERGS]()
				{
					ERGS->BuildTeamCache();
				});
		}
	}
}

void AER_InGameMode::EndGame()
{

}


void AER_InGameMode::NotifyPlayerDied(ACharacter* VictimCharacter, AActor* DeathCauser)
{
	if (!HasAuthority() || !VictimCharacter)
		return;

	UE_LOG(LogTemp, Warning, TEXT("[GM] : Start NotifyPlayerDied"));

	AER_PlayerState* ERPS = VictimCharacter->GetPlayerState<AER_PlayerState>();
	AER_GameState* ERGS = GetGameState<AER_GameState>();

	if (!ERPS || !ERGS)
		return;

	if (UER_RespawnSubsystem* RespawnSS = GetWorld()->GetSubsystem<UER_RespawnSubsystem>())
	{
		RespawnSS->HandlePlayerDeath(*ERPS, *ERGS);

		if (RespawnSS->EvaluateTeamElimination(*ERPS, *ERGS))
		{
			UE_LOG(LogTemp, Warning, TEXT("[GM] : NotifyPlayerDied , EvaluateTeamElimination = true"));

			// 전멸 판정 true -> 해당 유저의 팀 사출 실행
			const int32 TeamIdx = static_cast<int32>(ERPS->Team);

			// 해당 팀의 리스폰 타이머 정지
			RespawnSS->StopResapwnTimer(*ERGS, TeamIdx);

			// 해당 팀 패배 처리
			RespawnSS->SetTeamLose(*ERGS, TeamIdx);

			// 승리 팀 체크
			int32 LastTeamIdx = RespawnSS->CheckIsLastTeam(*ERGS);
			if (LastTeamIdx != -1)
			{
				RespawnSS->SetTeamWin(*ERGS, LastTeamIdx);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[GM] : NotifyPlayerDied , EvaluateTeamElimination = false"));

			// 전멸 판정 false -> 리스폰 함수 실행
			RespawnSS->StartRespawnTimer(*ERPS, *ERGS);
		}
	}
}

void AER_InGameMode::DisConnectClient(APlayerController* PC)
{
	if (GameSession)
	{
		GameSession->KickPlayer(PC, FText::FromString(TEXT("Defeated")));
	}
}
