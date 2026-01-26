// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ER_TitleGamePlayerController.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTER_API AER_TitleGamePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

protected:
    // 타이틀 UI 블루프린트에서 집어넣는거 잊지 말기
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
    TSubclassOf<class UUserWidget> TitleWidgetClass;

    UPROPERTY()
    class UUserWidget* CurrentTitleWidget;
	
};
