// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameModeBase/GameMode/ER_GameModeBase.h"
#include "ER_TitleGameMode.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTER_API AER_TitleGameMode : public AER_GameModeBase
{
	GENERATED_BODY()
public:
    AER_TitleGameMode();

protected:
    virtual void BeginPlay() override;


};
