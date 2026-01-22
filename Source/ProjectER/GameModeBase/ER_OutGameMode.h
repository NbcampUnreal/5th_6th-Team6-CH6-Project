// Fill out your copyright notice in the Description page of Project Settings.

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
	virtual void PostLogin(APlayerController* NewPlayer);
};
