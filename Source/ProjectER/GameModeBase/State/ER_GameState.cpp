#include "ER_GameState.h"
#include "GameModeBase/State/ER_PlayerState.h"
#include "GameModeBase/ER_OutGamePlayerController.h"


void AER_GameState::BuildTeamCache()
{
	if (!HasAuthority()) 
		return;

	UE_LOG(LogTemp, Warning, TEXT("[GS] : Start BuildTeamCache"));

	const int32 TeamCount = static_cast<int32>(ETeam::Team3);

	TeamCache.SetNum(TeamCount + 1);

	RemoveTeamCache();

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

	for (int32 TeamIdx = 0; TeamIdx <= TeamCount; ++TeamIdx)
	{
		if (!TeamCache.IsValidIndex(TeamIdx))
			continue;

		for (auto& it : TeamCache[TeamIdx])
		{
			UE_LOG(LogTemp, Log, TEXT("TeamIdx : %d | %s"), TeamIdx, *it->GetPlayerName());
		}
	}

	for (int32 TeamIdx = 1; TeamIdx <= TeamCount; ++TeamIdx)
	{
		if (!TeamCache[TeamIdx].IsEmpty())
		{
			TeamElimination.Add(TeamIdx, false);
		}
		else
		{
			TeamElimination.Add(TeamIdx, true);
		}
		UE_LOG(LogTemp, Log, TEXT("TeamElimination | %d | %s"), TeamIdx, *TeamElimination.Find(TeamIdx) ? TEXT("True") : TEXT("False"));
	}

}

void AER_GameState::RemoveTeamCache()
{
	if (!HasAuthority()) 
		return;

	UE_LOG(LogTemp, Warning, TEXT("[GS] : Start RemoveTeamCache"));
	for (auto& Team : TeamCache)
	{
		Team.Reset();
	}

	TeamElimination.Reset();
}

TArray<TWeakObjectPtr<AER_PlayerState>>& AER_GameState::GetTeamArray(int32 TeamIdx)
{
	return TeamCache[TeamIdx];
}

bool AER_GameState::GetTeamEliminate(int32 idx)
{
	UE_LOG(LogTemp, Warning, TEXT("[GS] : Start GetTeamEliminate"));

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
		return true;
	}
	else
	{
		return false;

	}
}

int32 AER_GameState::GetLastTeamIdx()
{
	UE_LOG(LogTemp, Warning, TEXT("[GS] : Start RemoveTeamCache"));

	int32 AliveTeamCount = 0;
	int32 AliveTeamIdx = -1;

	for (const auto& Team : TeamElimination)
	{
		if (Team.Value == false)
		{
			++AliveTeamCount;
			AliveTeamIdx = Team.Key;

			if (AliveTeamCount >= 2)
				return -1;
		}
	}

	return (AliveTeamCount == 1) ? AliveTeamIdx : -1;
	// 동시 탈락도 생각해야할 듯 추후에 개선
}

