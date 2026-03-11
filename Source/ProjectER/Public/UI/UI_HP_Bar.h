// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI_HP_Bar.generated.h"


class UTextBlock;
class UProgressBar;
class UImage;
class UAbilitySystemComponent;

/**
 * 
 */
UCLASS()
class PROJECTER_API UUI_HP_Bar : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UI_MainHUD")
	void Update_HP_bar(float CurrentHP, float MaxHP, int32 team);

	UFUNCTION(BlueprintCallable, Category = "UI_MainHUD")
	void Update_MP_bar(float CurrentMP, float MaxMP);

	UFUNCTION(BlueprintCallable, Category = "UI_MainHUD")
	void Update_LV_bar(float CurrentLV);

	UFUNCTION(BlueprintCallable, Category = "UI_MainHUD")
	void Update_HeadIcon(UTexture2D* NewIcon);

protected:
	UPROPERTY(meta = (BindWidget))
	UProgressBar* PB_HP;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* PB_MP;

	///  stat_nn = 임시명칭
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TXT_LV;

	UPROPERTY(meta = (BindWidget))
	UImage* IMG_Head;
};
