#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ER_InGameMode.generated.h"


UCLASS()
class PROJECTER_API AER_InGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	virtual void BeginPlay() override;
	virtual void PostSeamlessTravel() override;

	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	void EndGame();

	UFUNCTION(BlueprintCallable)
	void NotifyPlayerDied(ACharacter* VictimCharacter, AActor* DeathCauser);
};
