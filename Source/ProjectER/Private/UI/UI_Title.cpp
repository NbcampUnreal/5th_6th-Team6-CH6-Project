// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/UI_Title.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"

void UUI_Title::NativeConstruct()
{
    Super::NativeConstruct();

    if (IsValid(StartButton))
    {
        StartButton->OnClicked.AddDynamic(this, &UUI_Title::OnStartButtonClicked);
    }
}

void UUI_Title::OnStartButtonClicked()
{
    APlayerController* PC = GetOwningPlayer();
    if (PC)
    {
        FInputModeGameOnly InputMode;
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = false;
    }

    UGameplayStatics::OpenLevel(this, NAME_OF_MAINMENU);
}