#include "ER_PlayerState.h"
#include "Net/UnrealNetwork.h"

AER_PlayerState::AER_PlayerState()
{
	bReplicates = true;
}

void AER_PlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AER_PlayerState, PlayerStateName);
	DOREPLIFETIME(AER_PlayerState, bIsReady);
	DOREPLIFETIME(AER_PlayerState, bIsLose);
	DOREPLIFETIME(AER_PlayerState, bIsWin);
	DOREPLIFETIME(AER_PlayerState, bIsDead);
	DOREPLIFETIME(AER_PlayerState, Team);
	DOREPLIFETIME(AER_PlayerState, RespawnTime);

}

void AER_PlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	AER_PlayerState* PS = Cast<AER_PlayerState>(PlayerState);
	if (PS)
	{
		PS->PlayerStateName = PlayerStateName;
		PS->Team = Team;

	}
}

void AER_PlayerState::OnRep_IsLose()
{
	OnLoseChanged.Broadcast(bIsLose);
}

void AER_PlayerState::OnRep_IsWin()
{
	OnWinChanged.Broadcast(bIsWin);
}
