// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/UI_MainHUD.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"
#include "Components/SceneCaptureComponent2D.h" // 미니맵용
#include "Blueprint/SlateBlueprintLibrary.h" // 툴팁용
#include "Blueprint/WidgetLayoutLibrary.h" // 툴팁용
#include "UI/UI_ToolTip.h" // 툴팁용
#include "SkillSystem/SkillDataAsset.h" // 스킬용
#include "AbilitySystemComponent.h" // 스킬용
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

void UUI_MainHUD::InitHeroDataHUD(UCharacterData* _HeroData)
{
    HeroData = _HeroData;
}

void UUI_MainHUD::InitASCHud(UAbilitySystemComponent* _ASC)
{
    ASC = _ASC;
}

void UUI_MainHUD::NativeConstruct()
{
    Super::NativeConstruct();

    // 툴팁 init
    if (IsValid(TooltipClass) && !TooltipInstance)
    {
		TooltipInstance = Cast<UUI_ToolTip>(CreateWidget<UUserWidget>(GetWorld(), TooltipClass));
        TooltipInstance->SetVisibility(ESlateVisibility::Collapsed);
        TooltipInstance->AddToViewport(10); // UI 가시성 우선순위 위로
    }

    // 툴팁 바인딩
    if (skill_01)
    {
        skill_01->OnHovered.AddDynamic(this, &UUI_MainHUD::OnSkill01Hovered);
        skill_01->OnUnhovered.AddDynamic(this, &UUI_MainHUD::HideTooltip);
    }
    if (skill_02)
    {
        skill_02->OnHovered.AddDynamic(this, &UUI_MainHUD::OnSkill02Hovered);
        skill_02->OnUnhovered.AddDynamic(this, &UUI_MainHUD::HideTooltip);
    }
    if (skill_03)
    {
        skill_03->OnHovered.AddDynamic(this, &UUI_MainHUD::OnSkill03Hovered);
        skill_03->OnUnhovered.AddDynamic(this, &UUI_MainHUD::HideTooltip);
    }
    if (skill_04)
    {
        skill_04->OnHovered.AddDynamic(this, &UUI_MainHUD::OnSkill04Hovered);
        skill_04->OnUnhovered.AddDynamic(this, &UUI_MainHUD::HideTooltip);
    }
    // skil
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

void UUI_MainHUD::OnSkill01Hovered()
{
    // 차후 스킬 데이터 애셋에서 정보를 읽어올 수 있도록 개선해야 함
    ShowTooltip(skill_01, TEX_TempIcon, FText::FromString(TEXT("파이어볼")), FText::FromString(TEXT("화염 구체를 발사합니다.")), FText::FromString(TEXT("대미지: 100\n마나 소모: 50")), true);
}

void UUI_MainHUD::OnSkill02Hovered()
{
    ShowTooltip(skill_02, TEX_TempIcon, FText::FromString(TEXT("파이어볼파이어볼파이어볼파이어볼")), FText::FromString(TEXT("기분 좋은 해피 슈퍼 사연발 파이어볼을 해피하게 슈퍼메가 해피합니다. 울트라 해피.")), FText::FromString(TEXT("대미지: 100\n마나 소모: 50")), false);
}

void UUI_MainHUD::OnSkill03Hovered()
{
    ShowTooltip(skill_03, TEX_TempIcon, FText::FromString(TEXT("파이어볼 파이어볼")), FText::FromString(TEXT("화염 구체를 발사합니다.화염 구체를 발사합니다.화염 구체를 발사합니다.화염 구체를 발사합니다.화염 구체를 발사합니다.화염 구체를 발사합니다.")), FText::FromString(TEXT("대미지: 100\n마나 소모: 50")), true);
}

void UUI_MainHUD::OnSkill04Hovered()
{
    ShowTooltip(skill_04, TEX_TempIcon, FText::FromString(TEXT("파이어볼 파이어볼")), FText::FromString(TEXT("화염 구체를 발사합니다.\n화염 구체를 발사합니다.\n화염 구체를 발사합니다.\n화염 구체를 발사합니다.\n화염 구체를 발사합니다.\n화염 구체를 발사합니다.\n화염 구체를 발사합니다.\n화염 구체를 발사합니다.")), FText::FromString(TEXT("대미지: 100\n마나 소모: 50")), true);
}

void UUI_MainHUD::ShowTooltip(UWidget* AnchorWidget, UTexture2D* Icon, FText Name, FText ShortDesc, FText DetailDesc, bool showUpper)
{
    if (!TooltipInstance || !AnchorWidget) return;

    TooltipInstance->UpdateTooltip(Icon, Name, ShortDesc, DetailDesc);
    TooltipInstance->SetVisibility(ESlateVisibility::HitTestInvisible);

    // 위젯의 위치 쓰던 말던 일단 가져오기
    FGeometry WidgetGeom = AnchorWidget->GetCachedGeometry();
    FVector2D PixelPos, ViewportPos, FinalPos;
    FVector2D ButtonSize = AnchorWidget->GetDesiredSize();

    // 버튼의 왼쪽 상단 0, 0의 절대 좌표 가져오기
    USlateBlueprintLibrary::LocalToViewport(GetWorld(), WidgetGeom, FVector2D(0, 0), PixelPos, ViewportPos);

    // 툴 팁 크기
    TooltipInstance->ForceLayoutPrepass();
    FVector2D DesiredSize = TooltipInstance->GetDesiredSize();
    
    /// 가변 해상도를 고려한 크기 계산을 위해 반드시 DPI 스케일을 곱해줘야 한다!!!!!!!!!!!!!!!!!!!
    float DPIScale = UWidgetLayoutLibrary::GetViewportScale(TooltipInstance);
    FVector2D ActualSize = DesiredSize * DPIScale;

    //UE_LOG(LogTemp, Error, TEXT("PixelPos : %f, %f"), PixelPos.X, PixelPos.Y);
    //UE_LOG(LogTemp, Error, TEXT("ViewportPos : %f, %f"), ViewportPos.X, ViewportPos.Y);
    //UE_LOG(LogTemp, Error, TEXT("ActualSize Size : %f, %f"), ActualSize.X, ActualSize.Y);
    //UE_LOG(LogTemp, Error, TEXT("ButtonSize Size : %f, %f"), ButtonSize.X, ButtonSize.Y);
    
    FinalPos.X = PixelPos.X - (ActualSize.X / 2) + (ButtonSize.X / 2);
    if(showUpper)
        FinalPos.Y = PixelPos.Y - ActualSize.Y;
    else
		FinalPos.Y = PixelPos.Y + (ButtonSize.Y * 2);

    /// 툴팁이 밖으로 나갈 경우 안으로 들여보내기
    FVector2D ViewportSize;
    GEngine->GameViewport->GetViewportSize(ViewportSize);
    // 좌측 보정
    if (FinalPos.X + ActualSize.X > ViewportSize.X)
    {
        FinalPos.X = ViewportSize.X - ActualSize.X;
    }
    // 우측 보정
    if (FinalPos.X < 0.f)
    {
        FinalPos.X = 0.f;
    }

    // 상단 보정
    if (FinalPos.Y < 0.f)
    {
        // FinalPos.Y = 0.f;
        FinalPos.Y = PixelPos.Y + (ButtonSize.Y * 2);
    }
    // 하단 보정
    if (FinalPos.Y + ActualSize.Y > ViewportSize.Y)
    {
        // FinalPos.Y = ViewportSize.Y - ActualSize.Y;
        FinalPos.Y = PixelPos.Y - ActualSize.Y;
    }
    // 상단 하단의 경우 넘어서면 그냥 upper / lower를 토글하는 방식으로 처리하는게 이쁜것 갓다??
    // 주석 코드는 단순히 툴팁 크기만큼만 안 빠져 나가게 한다.

    TooltipInstance->SetPositionInViewport(FinalPos);
}

void UUI_MainHUD::HideTooltip()
{
    if (IsValid(TooltipInstance))
        TooltipInstance->SetVisibility(ESlateVisibility::Collapsed);
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
