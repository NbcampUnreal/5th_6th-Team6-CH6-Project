#include "GameModeBase/GameMode/ER_InGameMode.h"
#include "GameModeBase/State/ER_PlayerState.h"
#include "GameModeBase/State/ER_GameState.h"
#include "GameModeBase/Subsystem/Respawn/ER_RespawnSubsystem.h"

#include "GameFramework/Character.h"
#include "Engine/World.h"


void AER_InGameMode::EndGame()
{

}


void AER_InGameMode::NotifyPlayerDied(ACharacter* VictimCharacter, AActor* DeathCauser)
{
	if (!HasAuthority() || !VictimCharacter)
		return;

	AER_PlayerState* ERPS = VictimCharacter->GetPlayerState<AER_PlayerState>();
	AER_GameState* ERGS = GetGameState<AER_GameState>();

	if (!ERPS || !ERGS)
		return;

	if (UER_RespawnSubsystem* RespawnSS = GetWorld()->GetSubsystem<UER_RespawnSubsystem>())
	{
		RespawnSS->HandlePlayerDeath(*ERPS, *ERGS);

		if (RespawnSS->EvaluateTeamElimination(*ERPS, *ERGS))
		{
			// 전멸 판정 true일 시
			// 사출 함수 실행
		}
		else
		{
			// 전멸 판정 false일 시
			// 리스폰 함수 실행
		}
	}
}