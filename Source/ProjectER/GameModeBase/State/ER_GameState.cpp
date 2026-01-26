#include "ER_GameState.h"
#include "GameModeBase/State/ER_PlayerState.h"

void AER_GameState::BuildTeamCache()
{
	if (!HasAuthority()) 
		return;

	RemoveTeamCache();

	const int32 TeamCount = (int32)ETeam::Team3;

	for (int32 TeamIdx = 1; TeamIdx < TeamCount; ++TeamIdx)
	{
		TeamElimination.Add(TeamIdx, false);
		UE_LOG(LogTemp, Log, TEXT("TeamElimination | %d"), TeamIdx);
	}

	for (APlayerState* PS : PlayerArray)
	{
		if (AER_PlayerState* ERPS = Cast<AER_PlayerState>(PS))
		{
			const int32 TeamIdx = static_cast<int32>(ERPS->Team);

			if (!TeamCache.IsValidIndex(TeamIdx))
				continue;

			TeamCache[TeamIdx].AddUnique(ERPS);

			TeamElimination.FindOrAdd(TeamIdx) = false;
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

	TeamElimination.Empty();
}

void AER_GameState::CheckTeamEliminate(int32 idx)
{
	int32 AliveCount = 0;

	if (TeamCache.IsValidIndex(idx))
	{
		for (auto& WeakPS : TeamCache[idx])
		{
			AER_PlayerState* PS = WeakPS.Get();
			if (PS && !PS->bIsDead)
			{
				++AliveCount;
			}
		}
	}

	if (AliveCount == 0)
	{
		TeamElimination[idx] = true;
	}
}

