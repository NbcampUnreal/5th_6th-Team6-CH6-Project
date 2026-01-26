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
	void CheckTeamEliminate(int32 idx);

	// Getter
	bool GetTeamElimination(int32 idx) { return TeamElimination[idx]; }

public:
	TArray<TArray<TWeakObjectPtr<AER_PlayerState>>> TeamCache;

	UPROPERTY(BlueprintReadOnly)
	TMap<int32, bool> TeamElimination;
};
