// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModeBase/ER_TitleGamePlayerController.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Blueprint/UserWidget.h"

void AER_TitleGamePlayerController::BeginPlay()
{
    Super::BeginPlay();

    // 로컬 플레이어인지 확인 (서버의 대리인이 아닌 진짜 내 화면인지)
    if (IsLocalController())
    {
        if (TitleWidgetClass)
        {
            CurrentTitleWidget = CreateWidget<UUserWidget>(this, TitleWidgetClass);
            if (CurrentTitleWidget)
            {
                CurrentTitleWidget->AddToViewport();

                FInputModeUIOnly InputMode;
                InputMode.SetWidgetToFocus(CurrentTitleWidget->TakeWidget());
                SetInputMode(InputMode);
                bShowMouseCursor = true;
            }
        }
    }
}
