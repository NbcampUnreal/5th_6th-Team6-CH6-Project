// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI_Scoreboard.generated.h"

class UUI_ScoreboardPlayerRow; // << 핵심

class UScrollBox;
class UTextBlock;
class UButton;
class UProgressBar;
class UImage;
class UCharacterData;
class UAbilitySystemComponent;
class AER_GameState;
class AER_PlayerState;


UCLASS()
class PROJECTER_API UUI_Scoreboard : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(meta = (BindWidget))
	UScrollBox* SB_PlayerRow;
	UPROPERTY(meta = (BindWidget))
	UImage* TEX_FullMap;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUI_ScoreboardPlayerRow> RowWidgetClass;

	void NativeConstruct() override;


public:
	UFUNCTION()
	void UpdateScoreboard();

private:
	UPROPERTY()
	AER_GameState* GS;
	
};
