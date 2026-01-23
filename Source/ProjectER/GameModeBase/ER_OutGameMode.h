#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ER_OutGameMode.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTER_API AER_OutGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	void StartGame();
	
	void EndGame();

protected:
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;




public:

};
