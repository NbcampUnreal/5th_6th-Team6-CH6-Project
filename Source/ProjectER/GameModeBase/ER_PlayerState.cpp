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
}

