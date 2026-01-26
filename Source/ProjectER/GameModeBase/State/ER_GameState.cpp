#include "ER_GameState.h"
#include "GameModeBase/State/ER_PlayerState.h"

void AER_GameState::BuildTeamCache()
{
	if (!HasAuthority()) 
		return;

	for (APlayerState* PS : PlayerArray)
	{
		if (AER_PlayerState* ERPS = Cast<AER_PlayerState>(PS))
		{
			const int32 TeamIdx = static_cast<int32>(ERPS->Team);
			TeamCache[TeamIdx].AddUnique(ERPS);
		}
	}

}

void AER_GameState::RemoveTeamCache()
{
	if (!HasAuthority()) 
		return;

	for (auto& Team : TeamCache)
	{
		Team.Reset();
	}
}

