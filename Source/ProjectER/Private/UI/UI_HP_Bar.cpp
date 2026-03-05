// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/UI_HP_Bar.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"

void UUI_HP_Bar::Update_HP_bar(float CurrentHP, float MaxHP)
{
    if (IsValid(PB_HP))
    {
        float HealthPercent = (MaxHP > 0.f) ? (CurrentHP / MaxHP) : 0.f;
        PB_HP->SetPercent(HealthPercent);

        FLinearColor HealthColor;

        if (HealthPercent >= 0.6f) // 60% 이상
        {
            HealthColor = FLinearColor::Green;
        }
        else if (HealthPercent >= 0.3f) // 30% ~ 60% 미만
        {
            HealthColor = FLinearColor::Yellow;
        }
        else // 30% 미만
        {
            HealthColor = FLinearColor::Red;
        }

        PB_HP->SetFillColorAndOpacity(HealthColor);
    }
}

void UUI_HP_Bar::Update_MP_bar(float CurrentMP, float MaxMP)
{
    if (IsValid(PB_MP))
    {
        float ManaPercent = (MaxMP > 0.f) ? (CurrentMP / MaxMP) : 0.f;
        PB_MP->SetPercent(ManaPercent);

        // 마나/스태미너는 색깔 안변함
    }
}

void UUI_HP_Bar::Update_LV_bar(float CurrentLV)
{
    if (IsValid(TXT_LV))
    {
        TXT_LV->SetText(FText::AsNumber(FMath::RoundToInt(CurrentLV)));
    }
}