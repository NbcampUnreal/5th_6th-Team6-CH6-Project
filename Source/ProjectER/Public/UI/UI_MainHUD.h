// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI_MainHUD.generated.h"

class UTextBlock;
class UButton;
class UProgressBar;
class UImage;

UENUM(BlueprintType)
enum class ECharacterStat : uint8
{
	AD			UMETA(DisplayName = "Attack Damage"),
	AP			UMETA(DisplayName = "Ability Power"),
	AS			UMETA(DisplayName = "Attack Speed"),
	DEF			UMETA(DisplayName = "Defence"),
	ADEF		UMETA(DisplayName = "Ability Defence"),
	SPD			UMETA(DisplayName = "Speed")
};

/**
 * 
 */
UCLASS()
class PROJECTER_API UUI_MainHUD : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "UI_MainHUD")
	void Update_LV(float CurrentLV);
	UFUNCTION(BlueprintCallable, Category = "UI_MainHUD")
	void Update_XP(float CurrentXP, float MaxXP);
	UFUNCTION(BlueprintCallable, Category = "UI_MainHUD")
	void Update_HP(float CurrentHP, float MaxHP);
	UFUNCTION(BlueprintCallable, Category = "UI_MainHUD")
	void UPdate_MP(float CurrentHP, float MaxHP);
	UFUNCTION(BlueprintCallable, Category = "UI_MainHUD")
	void ShowSkillUp(bool show);

	UFUNCTION(BlueprintCallable, Category = "UI_MainHUD")
	void setStat(ECharacterStat stat, int32 Value);

protected:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* stat_LV;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* PB_XP;

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

	UPROPERTY(meta = (BindWidget))
	UImage* TEX_minamap;
};
