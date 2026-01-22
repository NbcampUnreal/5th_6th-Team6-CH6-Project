// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI_MainHUD.generated.h"

class UTextBlock;
class UButton;
class UProgressBar;

/**
 * 
 */
UCLASS()
class PROJECTER_API UUI_MainHUD : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "UI_MainHUD")
	void Update_HP(float CurrentHP, float MaxHP);
	UFUNCTION(BlueprintCallable, Category = "UI_MainHUD")
	void UPdate_MP(float CurrentHP, float MaxHP);
	UFUNCTION(BlueprintCallable, Category = "UI_MainHUD")
	void ShowSkillUp(bool show);

	UFUNCTION(BlueprintCallable, Category = "UI_MainHUD")
	void UPdate_LV(int32 lv);

protected:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* stat_LV;

	///  stat_nn = 임시명칭
	UPROPERTY(meta = (BindWidget))
	UTextBlock* stat_01;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* stat_02;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* stat_03;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* stat_04;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* stat_05;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* stat_06;

	UPROPERTY(meta = (BindWidget))
	UButton* skill_01;

	UPROPERTY(meta = (BindWidget))
	UButton* skill_02;

	UPROPERTY(meta = (BindWidget))
	UButton* skill_03;

	UPROPERTY(meta = (BindWidget))
	UButton* skill_04;

	UPROPERTY(meta = (BindWidget))
	UButton* skill_up_01;

	UPROPERTY(meta = (BindWidget))
	UButton* skill_up_02;

	UPROPERTY(meta = (BindWidget))
	UButton* skill_up_03;

	UPROPERTY(meta = (BindWidget))
	UButton* skill_up_04;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* PB_HP;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* PB_MP;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* current_HP;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* max_HP;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* current_MP;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* max_MP;
};
