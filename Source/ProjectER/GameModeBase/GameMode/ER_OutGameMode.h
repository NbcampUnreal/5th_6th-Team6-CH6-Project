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
	// mpyi _ 마우스 안보여서 마우스 보이게 하기 위하여 추가
	virtual void BeginPlay() override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	void StartGame();
	void EndGame();

	UFUNCTION(BlueprintCallable)
	void MoveTeam(APlayerController* Player, int32 TeamIdx);

protected:
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;




public:

};
