// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModeBase/ER_TitleGameMode.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Blueprint/UserWidget.h"

AER_TitleGameMode::AER_TitleGameMode()
{
}

void AER_TitleGameMode::BeginPlay()
{
    Super::BeginPlay();

    // UI는 서버에선 실행 안되게
    if (UKismetSystemLibrary::IsDedicatedServer(GetWorld()))
    {
        return;
    }

    APlayerController* PC = GetWorld()->GetFirstPlayerController();  
    if (PC && PC->IsLocalController() && TitleWidgetClass)
    {
        UUserWidget* TitleUI = CreateWidget<UUserWidget>(GetWorld(), TitleWidgetClass);
        if (TitleUI)
        {
            TitleUI->AddToViewport();

            FInputModeUIOnly InputMode;
            InputMode.SetWidgetToFocus(TitleUI->TakeWidget());
            PC->SetInputMode(InputMode);
            PC->bShowMouseCursor = true;
        }
    }
}
