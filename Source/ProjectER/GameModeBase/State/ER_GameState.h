#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ER_GameState.generated.h"


class AER_PlayerState;

UCLASS()
class PROJECTER_API AER_GameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	void BuildTeamCache();
	void RemoveTeamCache();

public:
	TArray<TArray<TWeakObjectPtr<AER_PlayerState>>> TeamCache;
};
