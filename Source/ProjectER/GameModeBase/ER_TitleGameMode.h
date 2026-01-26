// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameModeBase/ER_GameModeBase.h"
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

    // 타이틀 UI 블루프린트에서 집어넣는거 잊지 말기
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
    TSubclassOf<class UUserWidget> TitleWidgetClass;

    UPROPERTY()
    class UUserWidget* CurrentTitleWidget;
};
