#include "GameModeBase/GameMode/ER_InGameMode.h"
#include "GameModeBase/State/ER_PlayerState.h"
#include "GameModeBase/State/ER_GameState.h"
#include "GameModeBase/Subsystem/Respawn/ER_RespawnSubsystem.h"

#include "GameFramework/Character.h"
#include "Engine/World.h"


void AER_InGameMode::BeginPlay()
{
	Super::BeginPlay();
}

void AER_InGameMode::PostSeamlessTravel()
{
	Super::PostSeamlessTravel();
}

void AER_InGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	UE_LOG(LogTemp, Warning, TEXT("[GM] : HandleStartingNewPlayer called for %s"), NewPlayer ? *NewPlayer->GetName() : TEXT("nullptr"));

	AER_GameState* ERGS = GetGameState<AER_GameState>();
	if (ERGS)
	{
		// 다음 틱에 BuildTeamCache를 호출하여 모든 PlayerState가 완전히 복제되도록 함
		GetWorld()->GetTimerManager().SetTimerForNextTick([ERGS]()
			{
				ERGS->BuildTeamCache();
			});
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
			// 전멸 판정 true일 시
			// 사출 함수 실행
			RespawnSS->ShowLoseUI(*ERPS);

			// 만약 사출일 시 승리 팀 체크
			RespawnSS->CheckIsLastTeam(*ERGS);
	
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[GM] : NotifyPlayerDied , EvaluateTeamElimination = false"));
			// 전멸 판정 false일 시
			// 리스폰 함수 실행
		}
	}
}