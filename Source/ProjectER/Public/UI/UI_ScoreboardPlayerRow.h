// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI_ScoreboardPlayerRow.generated.h"

class UTextBlock;
class UButton;
class UProgressBar;
class UImage;
class UCharacterData;
class UAbilitySystemComponent;
class AER_GameState;
class AER_PlayerState;

UCLASS()
class PROJECTER_API UUI_ScoreboardPlayerRow : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* txtPlayerName;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* txtPlayerLV;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* txtPlayerKill;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* txtPlayerDeath;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* txtPlayerAssist;
	UPROPERTY(meta = (BindWidget))
	UProgressBar* PB_HP;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* txtHP;
	UPROPERTY(meta = (BindWidget))
	UImage* Img_Icon;
	UPROPERTY(meta = (BindWidget))
	UImage* Img_BG;

public:
	UFUNCTION()
	void Init(AER_PlayerState* PlayerState);
	UFUNCTION()
	void UpdatePlayerName(FText PlayerName);
	UFUNCTION()
	void UpdatePlayerLV(float PlayerLV);
	UFUNCTION()
	void UpdatePlayerKill(float PlayerKill);
	UFUNCTION()
	void UpdatePlayerDeath(float PlayerDeath);
	UFUNCTION()
	void UpdatePlayerAssist(float PlayerAssist);
	UFUNCTION()
	void UpdatePlayerHP(float CurrentHP, float MaxHP);
	UFUNCTION()
	void UpdatePlayerIcon(UTexture2D* Icon);
	UFUNCTION()
	void UpdatePlayerBG(FLinearColor BGColor);
	
};
