// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/UI_MainHUD.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"

void UUI_MainHUD::Update_HP(float CurrentHP, float MaxHP)
{
    if (IsValid(PB_HP))
    {
        float HealthPercent = CurrentHP / MaxHP;
        PB_HP->SetPercent(HealthPercent);
    }

    if (IsValid(current_HP))
    {
		current_HP->SetText(FText::AsNumber(FMath::RoundToInt(CurrentHP)));
    }
}

void UUI_MainHUD::UPdate_MP(float CurrentMP, float MaxMP)
{
    if (IsValid(PB_MP))
    {
        float HealthPercent = CurrentMP / MaxMP;
        PB_MP->SetPercent(HealthPercent);
    }

    if (IsValid(current_MP))
    {
        current_MP->SetText(FText::AsNumber(FMath::RoundToInt(CurrentMP)));
    }
}

void UUI_MainHUD::ShowSkillUp(bool show)
{
    if (IsValid(skill_up_01))
    {
        skill_up_01->SetVisibility(show ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    }
    if (IsValid(skill_up_02))
    {
        skill_up_02->SetVisibility(show ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    }
    if (IsValid(skill_up_03))
    {
        skill_up_03->SetVisibility(show ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    }
    if (IsValid(skill_up_04))
    {
        skill_up_04->SetVisibility(show ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}
