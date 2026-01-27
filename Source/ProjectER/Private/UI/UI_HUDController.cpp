// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/UI_HUDController.h"
#include "CharacterSystem/Player/BasePlayerState.h"
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"

void UUI_HUDController::SetParams(const FWidgetControllerParams& Params)
{
	PlayerController = Params.PlayerController;
	PlayerState = Params.PlayerState;
	AbilitySystemComponent = Params.AbilitySystemComponent;
	AttributeSet = Params.AttributeSet;
}

void UUI_HUDController::BindCallbacksToDependencies()
{
    UBaseAttributeSet* BaseAS = CastChecked<UBaseAttributeSet>(AttributeSet);

    // HP 변경 감지 람다
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(BaseAS->GetHealthAttribute()).AddLambda(
        [this](const FOnAttributeChangeData& Data)
        {
            // UE_LOG(LogTemp, Log, TEXT("[Delegate] HP 변경 감지됨: %f"), Data.NewValue);

            float CurrentMaxHP = CastChecked<UBaseAttributeSet>(AttributeSet)->GetMaxHealth();
            BroadcastHPChanges(Data.NewValue, CurrentMaxHP);
        }
    );

    // MaxHP 변경 감지 람다
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(BaseAS->GetMaxHealthAttribute()).AddLambda(
        [this](const FOnAttributeChangeData& Data)
        {
            // UE_LOG(LogTemp, Log, TEXT("[Delegate] MaxHP 변경 감지됨: %f"), Data.NewValue);

            float CurrentHP = CastChecked<UBaseAttributeSet>(AttributeSet)->GetHealth();
            BroadcastHPChanges(CurrentHP, Data.NewValue);
        }
    );

    // 차후 위 람다식을 모든 스탯에 대해 반복 해야함~
}

// 실제 체력 변화 브로드캐스트
void UUI_HUDController::BroadcastHPChanges(float CurrentHP, float MaxHP)
{
    if (IsValid(MainHUDWidget))
    {
        // UE_LOG(LogTemp, Warning, TEXT("[Broadcast] 위젯으로 전송 중: HP(%f), MaxHP(%f)"), CurrentHP, MaxHP);
        MainHUDWidget->Update_HP(CurrentHP, MaxHP);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[Broadcast] 실패: MainHUDWidget이 유효하지 않음!"));
    }
}