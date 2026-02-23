// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/UI_HUDFactory.h"
#include "Blueprint/UserWidget.h"
#include "UI/UI_HUDController.h"
#include "SkillSystem/SkillDataAsset.h"
#include "AbilitySystemComponent.h" // 스킬용
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"

void AUI_HUDFactory::BeginPlay()
{
    //if (MainHUDWidgetClass)
    //{
    //    MainWidget = CreateWidget<UUI_MainHUD>(GetOwningPlayerController(), MainHUDWidgetClass);
    //    MainWidget->AddToViewport();
    //}

    //if (WidgetControllerClass)
    //{
    //    WidgetController = NewObject<UUI_HUDController>(this, WidgetControllerClass);

    //    WidgetController->MainHUDWidget = MainWidget;
    //}

    /// 위 내용을 이닛오버레이에서 하도록 변경!!!!!!!!!!!!!!!
}

void AUI_HUDFactory::InitOverlay(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
    if (WidgetControllerClass == nullptr) return;
    if (MainHUDWidgetClass == nullptr) return;

    // 혹시 모를 중복 방지로 기존에 있었으면 지움
    if (MainWidget)
    {
        MainWidget->RemoveFromParent();
    }

    // 컨트롤러 생성 및 데이터 설정
    WidgetController = NewObject<UUI_HUDController>(this, WidgetControllerClass);
    FWidgetControllerParams Params{ PC, PS, ASC, AS };
    WidgetController->SetParams(Params);

    // 위젯 생성 및 컨트롤러 연결
    MainWidget = CreateWidget<UUI_MainHUD>(GetWorld(), MainHUDWidgetClass);
    WidgetController->MainHUDWidget = MainWidget;

    MainWidget->AddToViewport();
    WidgetController->BindCallbacksToDependencies();

    // 초기값 강제 전송 (중요!!!!!!!!!!!!!!!)
    // 지금은 체력만 만들어놨지만 모든 스탯 추가할것!!!!!!!!!!!
    if (UBaseAttributeSet* BaseAS = Cast<UBaseAttributeSet>(AS))
    {
        WidgetController->BroadcastLVChanges(BaseAS->GetLevel());
        WidgetController->BroadcastHPChanges(BaseAS->GetHealth(), BaseAS->GetMaxHealth());
        WidgetController->BroadcastStaminaChanges(BaseAS->GetStamina(), BaseAS->GetMaxStamina());
        WidgetController->BroadcastXPChanges(BaseAS->GetXP(), BaseAS->GetMaxXP());
        WidgetController->BroadcastATKChanges(BaseAS->GetAttackPower());
        WidgetController->BroadcastSPChanges(BaseAS->GetSkillAmp());
        WidgetController->BroadcastDEFChanges(BaseAS->GetDefense());
        WidgetController->BroadcastCooldownReduction(BaseAS->GetCooldownReduction());
        WidgetController->BroadcastASChanges(BaseAS->GetAttackSpeed());
        WidgetController->BroadcastARChanges(BaseAS->GetAttackRange());
        WidgetController->BroadcastCCChanges(BaseAS->GetCriticalChance());
        WidgetController->BroadcastSpeedChanges(BaseAS->GetMoveSpeed());
    }
}

void AUI_HUDFactory::InitMinimapComponent(USceneCaptureComponent2D* SceneCapture2D)
{
    if (IsValid(MainWidget))
    {
        MainWidget->InitMinimapCompo(SceneCapture2D);
    }
}

void AUI_HUDFactory::InitHeroDataFactory(UCharacterData* HeroData)
{
    if (HeroData)
    {
        MainWidget->InitHeroDataHUD(HeroData);
    }
}

void AUI_HUDFactory::InitASCFactory(UAbilitySystemComponent* _ASC)
{
    if (_ASC)
    {
        MainWidget->InitASCHud(_ASC);
    }
}

UUI_MainHUD* AUI_HUDFactory::getMainHUD()
{    
    if (IsValid(MainWidget))
    {
        return MainWidget;
    }

    return nullptr;
}
