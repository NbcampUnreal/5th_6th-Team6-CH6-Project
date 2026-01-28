// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/UI_MainHUD.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"

void UUI_MainHUD::Update_LV(float CurrentLV)
{
    if(IsValid(stat_LV))
    {
        stat_LV->SetText(FText::AsNumber(FMath::RoundToInt(CurrentLV)));
	}
}

void UUI_MainHUD::Update_XP(float CurrentXP, float MaxXP)
{
    if (IsValid(PB_XP))
    {
        float HealthPercent = CurrentXP / MaxXP;
        PB_XP->SetPercent(HealthPercent);

        // 디버깅용 색 변화
        PB_XP->SetFillColorAndOpacity(FLinearColor::MakeRandomColor());
    }
}

void UUI_MainHUD::Update_HP(float CurrentHP, float MaxHP)
{
    if (IsValid(PB_HP))
    {
        float HealthPercent = CurrentHP / MaxHP;
        PB_HP->SetPercent(HealthPercent);

        // 디버깅용 색 변화
        PB_HP->SetFillColorAndOpacity(FLinearColor::MakeRandomColor());
    }

    if (IsValid(current_HP))
    {
		current_HP->SetText(FText::AsNumber(FMath::RoundToInt(CurrentHP)));

        // 디버깅용 색 변화
        current_HP->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
    }

    if (IsValid(max_HP))
    {
        max_HP->SetText(FText::AsNumber(FMath::RoundToInt(MaxHP)));
    }
}

void UUI_MainHUD::UPdate_MP(float CurrentMP, float MaxMP)
{
    if (IsValid(PB_MP))
    {
        float HealthPercent = CurrentMP / MaxMP;
        PB_MP->SetPercent(HealthPercent);

        // 디버깅용 색 변화
        PB_MP->SetFillColorAndOpacity(FLinearColor::MakeRandomColor());
    }

    if (IsValid(current_MP))
    {
        current_MP->SetText(FText::AsNumber(FMath::RoundToInt(CurrentMP)));

        // 디버깅용 색 변화
        current_MP->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
    }

    if (IsValid(max_MP))
    {
        max_MP->SetText(FText::AsNumber(FMath::RoundToInt(MaxMP)));
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

void UUI_MainHUD::setStat(ECharacterStat stat, int32 value)
{
    if (stat == ECharacterStat::AD)
    {
        if (IsValid(stat_01))
        {
            stat_01->SetText(FText::AsNumber(value));
        }
    }
    else if (stat == ECharacterStat::AP)
    {
        if (IsValid(stat_02))
        {
            stat_02->SetText(FText::AsNumber(value));
        }
    }
    else if (stat == ECharacterStat::AS)
    {
        if (IsValid(stat_03))
        {
            stat_03->SetText(FText::AsNumber(value));
        }
    }
    else if (stat == ECharacterStat::DEF)
    {
        if (IsValid(stat_04))
        {
            stat_04->SetText(FText::AsNumber(value));
        }
    }
    else if (stat == ECharacterStat::ADEF)
    {
        if (IsValid(stat_05))
        {
            stat_05->SetText(FText::AsNumber(value));
        }
    }
    else if (stat == ECharacterStat::SPD)
    {
        if (IsValid(stat_06))
        {
            stat_06->SetText(FText::AsNumber(value));
        }
    }
}
