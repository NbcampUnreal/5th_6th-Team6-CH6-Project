// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/UI_HP_Bar.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"

void UUI_HP_Bar::Update_HP_bar(float CurrentHP, float MaxHP, int32 team)
{
    //
    // team == 0 : 나
    // team == 1 : 팀
    // team == 2 : 적
    //

    if (IsValid(PB_HP))
    {
        float HealthPercent = (MaxHP > 0.f) ? (CurrentHP / MaxHP) : 0.f;
        PB_HP->SetPercent(HealthPercent);

        FLinearColor HealthColor;
        if (team == 0)
        {
            // 본인 체력 항상 초록색
            HealthColor = FLinearColor::Green;

            //if (HealthPercent >= 0.6f) // 60% 이상
            //{
            //    HealthColor = FLinearColor::Green;
            //}
            //else if (HealthPercent >= 0.3f) // 30% ~ 60% 미만
            //{
            //    HealthColor = FLinearColor::Yellow;
            //}
            //else // 30% 미만
            //{
            //    HealthColor = FLinearColor::Red;
            //}
        }
        else if (team == 1)
        {
            HealthColor = FLinearColor::Yellow;
        }
        else if (team == 2)
        {
			HealthColor = FLinearColor::Red;
        }
        else
        {
            HealthColor = FLinearColor::Black;
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

void UUI_HP_Bar::Update_HeadIcon(UTexture2D* NewIcon)
{
    if (IsValid(IMG_Head))
    {
		IMG_Head->SetBrushFromTexture(NewIcon);
    }
}

void UUI_HP_Bar::Update_PlayerName(FText PlayerName)
{
    if(IsValid(TXT_PlayerName))
    {
        TXT_PlayerName->SetText(PlayerName);
	}
}

void UUI_HP_Bar::NativeConstruct()
{
    Super::NativeConstruct();
    if (IsValid(IMG_Head))
    {
        IMG_Head->SetVisibility(ESlateVisibility::Hidden);
    }
}
