// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/UI_MainHUD.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"
#include "Components/SceneCaptureComponent2D.h" // 미니맵용

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

void UUI_MainHUD::InitMinimapCompo(USceneCaptureComponent2D* SceneCapture2D)
{
    MinimapCaptureComponent = SceneCapture2D;
}

/// 마우스 이벤트!
FReply UUI_MainHUD::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // 우클릭인지 확인
    if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        if (IsValid(TEX_Minimap))
        {
            // 마우스 위치가 미니맵 내부 맞음?
            FGeometry MapGeom = TEX_Minimap->GetCachedGeometry();
            FVector2D LocalPos = MapGeom.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
            FVector2D Size = MapGeom.GetLocalSize();

            if (LocalPos.X >= 0 && LocalPos.X <= Size.X && LocalPos.Y >= 0 && LocalPos.Y <= Size.Y)
            {
                // 좌표 계산 및 이동 로직
                HandleMinimapClicked(InMouseEvent);

                // 이벤트 핸들 처리
                return FReply::Handled();
            }
        }
    }

    // 미니맵 영역 밖이면 UI 안 만진걸로 처리
    return FReply::Unhandled();
}

void UUI_MainHUD::HandleMinimapClicked(const FPointerEvent& InMouseEvent)
{
    if (!IsValid(TEX_Minimap) || !IsValid(MinimapCaptureComponent)) return;

    FGeometry MapGeometry = TEX_Minimap->GetCachedGeometry();
    FVector2D LocalClickPos = MapGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
    FVector2D ImageSize = MapGeometry.GetLocalSize();

    /// 실제 클릭 위치 좌표 구하기
    float AlphaX = LocalClickPos.X / ImageSize.X;
    float AlphaY = LocalClickPos.Y / ImageSize.Y;
    float OffsetXRatio = AlphaX - 0.5f;
    float OffsetYRatio = AlphaY - 0.5f;

    // 씬캡쳐의 OrthoWidth를 기준으로 실제 월드 단위 거리 계산
    float MapWidth = MinimapCaptureComponent->OrthoWidth;

    // 캐릭터로부터의 상대 거리 계산
    float RelativeX = -(OffsetYRatio * MapWidth);
    float RelativeY = (OffsetXRatio * MapWidth);

    // 최종 목적지 = 현재 캐릭터 위치 + 상대 거리
    APawn* PlayerPawn = GetOwningPlayerPawn();
    if (IsValid(PlayerPawn))
    {
        FVector CurrentCharLoc = PlayerPawn->GetActorLocation();
        FVector TargetWorldPos = CurrentCharLoc + FVector(RelativeX, RelativeY, 0.f);

        // 결과 확인용 로그
        UE_LOG(LogTemp, Error, TEXT("Relative : X=%f, Y=%f"), RelativeX, RelativeY);
        UE_LOG(LogTemp, Error, TEXT("Absolute : %s"), *TargetWorldPos.ToString());

        // 이동 명령
        // MoveToLocation(TargetWorldPos);
    }
}
