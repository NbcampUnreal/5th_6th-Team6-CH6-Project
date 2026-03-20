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

    // LV 변경 감지 람다
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
        BaseAS->GetLevelAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
            {
                BroadcastLVChanges(Data.NewValue);
            }
        );

    // XP 변경 감지 람다
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
        BaseAS->GetXPAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
            {
                float CurrentMaxXP = CastChecked<UBaseAttributeSet>(AttributeSet)->GetMaxXP();
                BroadcastXPChanges(Data.NewValue, CurrentMaxXP);
            }
        );

    // HP 변경 감지 람다
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
        BaseAS->GetHealthAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
        {
            // UE_LOG(LogTemp, Log, TEXT("[Delegate] HP 변경 감지됨: %f"), Data.NewValue);

            float CurrentMaxHP = CastChecked<UBaseAttributeSet>(AttributeSet)->GetMaxHealth();
            BroadcastHPChanges(Data.NewValue, CurrentMaxHP);
        }
    );

    // MaxHP 변경 감지 람다
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
        BaseAS->GetMaxHealthAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
        {
            float CurrentHP = CastChecked<UBaseAttributeSet>(AttributeSet)->GetHealth();
            BroadcastHPChanges(CurrentHP, Data.NewValue);
        }
    );
    // MP 변경 감지 람다
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
        BaseAS->GetStaminaAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
            {
                float CurrentMaxMP = CastChecked<UBaseAttributeSet>(AttributeSet)->GetMaxStamina();
                BroadcastStaminaChanges(Data.NewValue, CurrentMaxMP);
            }
        );

    // MaxMP 변경 감지 람다
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
        BaseAS->GetMaxStaminaAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
            {
                float CurrentMP = CastChecked<UBaseAttributeSet>(AttributeSet)->GetStamina();
                BroadcastStaminaChanges(CurrentMP, Data.NewValue);
            }
        );

    // ATK 변경 감지 람다
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
        BaseAS->GetAttackPowerAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
            {
                BroadcastATKChanges(Data.NewValue);
            }
        );
    
    // AS 변경 감지 람다
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
        BaseAS->GetAttackSpeedAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
            {
                BroadcastASChanges(Data.NewValue);
            }
        );

    // DEF 변경 감지 람다
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
        BaseAS->GetDefenseAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
            {
                BroadcastDEFChanges(Data.NewValue);
            }
        );

    // SkillAmp 변경 감지 람다
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
        BaseAS->GetSkillAmpAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
            {
                BroadcastSPChanges(Data.NewValue);
            }
        );

    // CC 변경 감지 람다
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
        BaseAS->GetCriticalChanceAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
            {
                BroadcastCCChanges(Data.NewValue);
            }
        );

    // Speed 변경 감지 람다
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
        BaseAS->GetMoveSpeedAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
            {
                BroadcastSpeedChanges(Data.NewValue);
            }
        );
    // Cool 변경 감지 람다
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
        BaseAS->GetCooldownReductionAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
            {
                BroadcastCooldownReduction(Data.NewValue);
            }
        );
    // AttackRange 변경 감지 람다
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
        BaseAS->GetAttackRangeAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
            {
				BroadcastARChanges(Data.NewValue);
            }
        );
    // 스킬 포인트 변경 감지 람다
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
        BaseAS->GetSkillPointAttribute()).AddLambda([this](const FOnAttributeChangeData& Data)
            {
                BroadcastSkillPointChanges(Data.NewValue);
            }
        );
    // 차후 위 람다식을 모든 스탯에 대해 반복 해야함~
}

void UUI_HUDController::BroadcastLVChanges(float CurrentLV)
{
    if (IsValid(MainHUDWidget))
    {
        MainHUDWidget->Update_LV(CurrentLV);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Broadcast] 실패: MainHUDWidget이 유효하지 않음!"));
    }
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
        UE_LOG(LogTemp, Warning, TEXT("[Broadcast] 실패: MainHUDWidget이 유효하지 않음!"));
    }
}

void UUI_HUDController::BroadcastStaminaChanges(float CurrentST, float MaxST)
{
    if (IsValid(MainHUDWidget))
    {
        MainHUDWidget->UPdate_MP(CurrentST, MaxST);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Broadcast] 실패: MainHUDWidget이 유효하지 않음!"));
    }
}

void UUI_HUDController::BroadcastXPChanges(float CurrentXP, float MaxXP)
{
    if (IsValid(MainHUDWidget))
    {
        MainHUDWidget->Update_XP(CurrentXP, MaxXP);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Broadcast] 실패: MainHUDWidget이 유효하지 않음!"));
    }
}

void UUI_HUDController::BroadcastATKChanges(float CurrentATK)
{    
    if (IsValid(MainHUDWidget))
    {
        MainHUDWidget->setStat(ECharacterStat::AD, CurrentATK);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Broadcast] 실패: MainHUDWidget이 유효하지 않음!"));
    }
}

void UUI_HUDController::BroadcastSPChanges(float CurrentSP)
{
    if (IsValid(MainHUDWidget))
    {
        MainHUDWidget->setStat(ECharacterStat::AP, CurrentSP);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Broadcast] 실패: MainHUDWidget이 유효하지 않음!"));
    }
}

void UUI_HUDController::BroadcastASChanges(float CurrentAS)
{
    if (IsValid(MainHUDWidget))
    {
        MainHUDWidget->setStat(ECharacterStat::AS, CurrentAS);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Broadcast] 실패: MainHUDWidget이 유효하지 않음!"));
    }
}

void UUI_HUDController::BroadcastARChanges(float CurrentAR)
{
    if (IsValid(MainHUDWidget))
    {
        MainHUDWidget->setStat(ECharacterStat::ATRAN, CurrentAR);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Broadcast] 실패: MainHUDWidget이 유효하지 않음!"));
    }
}

void UUI_HUDController::BroadcastCCChanges(float CurrentCC)
{
    if (IsValid(MainHUDWidget))
    {
        MainHUDWidget->setStat(ECharacterStat::CC, CurrentCC);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Broadcast] 실패: MainHUDWidget이 유효하지 않음!"));
    }
}

void UUI_HUDController::BroadcastDEFChanges(float CurrentDEF)
{
    if (IsValid(MainHUDWidget))
    {
        MainHUDWidget->setStat(ECharacterStat::DEF, CurrentDEF);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Broadcast] 실패: MainHUDWidget이 유효하지 않음!"));
    }
}

void UUI_HUDController::BroadcastSpeedChanges(float CurrentSpeed)
{
    if (IsValid(MainHUDWidget))
    {
        MainHUDWidget->setStat(ECharacterStat::SPD, CurrentSpeed);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Broadcast] 실패: MainHUDWidget이 유효하지 않음!"));
    }
}
void UUI_HUDController::BroadcastCooldownReduction(float Cooldown)
{
    if (IsValid(MainHUDWidget))
    {
        MainHUDWidget->setStat(ECharacterStat::COOL, Cooldown);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Broadcast] 실패: MainHUDWidget이 유효하지 않음!"));
    }
}

void UUI_HUDController::BroadcastSkillPointChanges(float _SkillPoint)
{
    if (IsValid(MainHUDWidget))
    {
        MainHUDWidget->UpdateSkillPoint(_SkillPoint);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Broadcast] 실패: MainHUDWidget이 유효하지 않음!"));
    }
}
