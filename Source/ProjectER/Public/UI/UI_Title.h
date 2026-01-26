// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI_Title.generated.h"

#define NAME_OF_MAINMENU FName("Level_MainMenu")
/**
 * 
 */
UCLASS()
class PROJECTER_API UUI_Title : public UUserWidget
{
	GENERATED_BODY()
	

protected:
    UPROPERTY(meta = (BindWidget))
	class UImage* TitleImage;

    UPROPERTY(meta = (BindWidget))
    class UButton* StartButton;

    virtual void NativeConstruct() override;

    UFUNCTION()
    void OnStartButtonClicked();
};
