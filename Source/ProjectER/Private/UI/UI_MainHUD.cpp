// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/UI_MainHUD.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"
#include "Components/SceneCaptureComponent2D.h" // 미니맵용
#include "NavigationSystem.h" // 미니맵용
#include "NavigationPath.h" // 미니맵용

#include "Blueprint/SlateBlueprintLibrary.h" // 툴팁용
#include "Blueprint/WidgetLayoutLibrary.h" // 툴팁용
#include "UI/UI_ToolTip.h" // 툴팁용
#include "SkillSystem/SkillConfig/BaseSkillConfig.h"

#include "CharacterSystem/Character/BaseCharacter.h"

#include "SkillSystem/SkillDataAsset.h" // 스킬용
#include "AbilitySystemComponent.h" // 스킬용
#include "CharacterSystem/Data/CharacterData.h" // 스킬용
#include "SkillSystem/SkillDataAsset.h"
#include "SkillSystem/SkillConfig/BaseSkillConfig.h"
#include "Kismet/KismetMathLibrary.h" // 반올림용
#include "CharacterSystem/Player/BasePlayerController.h"
#include "Abilities/GameplayAbilityTypes.h" // 쿨타임용

#include "Blueprint/WidgetBlueprintGeneratedClass.h" // 초상화 반짝 애니메이션용
#include "Animation/WidgetAnimation.h" // 초상화 반짝 애니메이션용
#include "MovieScene.h" // 초상화 반짝 애니메이션용

#include "Kismet/GameplayStatics.h" // gamestate용
#include "GameModeBase/State/ER_GameState.h" // gamestate
#include "GameModeBase/State/ER_PlayerState.h"

#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"  // AttributeSet용

// 인벤토리 UI용
#include "Components/UniformGridPanel.h"
#include "ItemSystem/Component/BaseInventoryComponent.h"
#include "ItemSystem/Data/BaseItemData.h"
#include "Components/UniformGridSlot.h"
#include "ItemSystem/UI/W_InventorySlot.h"
void UUI_MainHUD::Update_LV(float CurrentLV)
{
    if(IsValid(stat_LV))
    {
        stat_LV->SetText(FText::AsNumber(FMath::RoundToInt(CurrentLV)));
		nowLevel = CurrentLV;
	}
}

void UUI_MainHUD::Update_XP(float CurrentXP, float MaxXP)
{
    if (IsValid(PB_XP))
    {
        float HealthPercent = CurrentXP / MaxXP;
        PB_XP->SetPercent(HealthPercent);

        // 디버깅용 색 변화
        // PB_XP->SetFillColorAndOpacity(FLinearColor::MakeRandomColor());
    }
}

void UUI_MainHUD::Update_HP(float CurrentHP, float MaxHP)
{
    if (IsValid(PB_HP))
    {
        float HealthPercent = CurrentHP / MaxHP;
        PB_HP->SetPercent(HealthPercent);

        // 체력 비율에 따른 색 변화
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

    if (IsValid(current_HP))
    {
		current_HP->SetText(FText::AsNumber(FMath::RoundToInt(CurrentHP)));

        // 디버깅용 색 변화
        // current_HP->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
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
        // PB_MP->SetFillColorAndOpacity(FLinearColor::MakeRandomColor());
    }

    if (IsValid(current_MP))
    {
        current_MP->SetText(FText::AsNumber(FMath::RoundToInt(CurrentMP)));

        // 디버깅용 색 변화
        // current_MP->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
    }

    if (IsValid(max_MP))
    {
        max_MP->SetText(FText::AsNumber(FMath::RoundToInt(MaxMP)));
    }
}

void UUI_MainHUD::ShowSkillUp(bool show, bool isUlt/* = false */)
{
    if (UI_BACKGROUND_LevelUp && UI_BACKGROUND_LevelUp_Ult)
    {
       if (show)
       {
           if (isUlt)
               UI_BACKGROUND_LevelUp_Ult->SetVisibility(ESlateVisibility::Visible);
           else
               UI_BACKGROUND_LevelUp->SetVisibility(ESlateVisibility::Visible);
       }
       else
       {
            UI_BACKGROUND_LevelUp_Ult->SetVisibility(ESlateVisibility::Hidden);
            UI_BACKGROUND_LevelUp->SetVisibility(ESlateVisibility::Hidden);
            skill_up_04->SetVisibility(ESlateVisibility::Hidden);
       }
    }

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
    if (isUlt)
    {
        if (IsValid(skill_up_04))
        {
            skill_up_04->SetVisibility(show ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
        }
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
    else if (stat == ECharacterStat::DEF)
    {
        if (IsValid(stat_03))
        {
            stat_03->SetText(FText::AsNumber(value));
        }
    }
    else if (stat == ECharacterStat::COOL)
    {
        if (IsValid(stat_04))
        {
            stat_04->SetText(FText::AsNumber(value));
        }
    }
    else if (stat == ECharacterStat::AS)
    {
        if (IsValid(stat_05))
        {
            stat_05->SetText(FText::AsNumber(value));
        }
    }
    else if (stat == ECharacterStat::ATRAN)
    {
        if (IsValid(stat_06))
        {
            stat_06->SetText(FText::AsNumber(value));
        }
    }
    else if (stat == ECharacterStat::CC)
    {
        if (IsValid(stat_07))
        {
            stat_07->SetText(FText::AsNumber(value));
            nowSkillCoolReduc = value;
        }
    }
    else if (stat == ECharacterStat::SPD)
    {
        if (IsValid(stat_07))
        {
            stat_08->SetText(FText::AsNumber(value));
        }
    }
}

void UUI_MainHUD::UpdateSkillPoint(float _nowSP)
{
    if (_nowSP < 1)
    {
		ShowSkillUp(false);

    }
    else
    {
        bool isOverUltLevel = false;

        int nowUltLevel = getSkillLevel(R_SkillTag, false);
        if (nowLevel >= 3 && nowUltLevel < 1)
        {
            isOverUltLevel = true;
        }
        if (nowLevel >= 6 && nowUltLevel < 2)
        {
            isOverUltLevel = true;
        }
        if (nowLevel >= 9 && nowUltLevel < 3)
        {
            isOverUltLevel = true;
        }
            



        ShowSkillUp(true, isOverUltLevel);
    }
	// UE_LOG(LogTemp, Error, TEXT("UpdateSkillPoint called with SP: %f"), _nowSP);
}

void UUI_MainHUD::InitMinimapCompo(USceneCaptureComponent2D* SceneCapture2D)
{
    MinimapCaptureComponent = SceneCapture2D;
}

void UUI_MainHUD::InitHeroDataHUD(UCharacterData* _HeroData)
{
    HeroData = _HeroData;
    initSkillDataAssets();
}

void UUI_MainHUD::InitASCHud(UAbilitySystemComponent* _ASC)
{
    ASC = _ASC;
    if (IsValid(ASC))
    {
        ASC->AbilityActivatedCallbacks.AddUObject(this, &UUI_MainHUD::OnAbilityActivated);
    }
    
}

void UUI_MainHUD::StartRespawn(float _RespawnTime)
{
	UE_LOG(LogTemp, Warning, TEXT("StartRespawn called with time: %f"), _RespawnTime);
}

void UUI_MainHUD::NativeConstruct()
{
    Super::NativeConstruct();

    // [김현수 추가분]Grid_item이 BindWidget으로 바인딩 안됐으면 직접 찾기
    if (!Grid_item)
    {
        UE_LOG(LogTemp, Warning, TEXT("[UI_MainHUD] Grid_item not bound, trying to find manually..."));
        Grid_item = Cast<UUniformGridPanel>(GetWidgetFromName(TEXT("Grid_item")));

        if (Grid_item)
        {
            UE_LOG(LogTemp, Warning, TEXT("[UI_MainHUD] Grid_item found manually!"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[UI_MainHUD] Grid_item not found even manually!"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[UI_MainHUD] Grid_item already bound via BindWidget!"));
    }

    EnsureInventorySlotWidgets();
    RefreshInventoryGridLayout();
    UpdateInventoryUI();

    if (UI_BACKGROUND_LevelUp && UI_BACKGROUND_LevelUp_Ult)
    {
        UI_BACKGROUND_LevelUp->SetVisibility(ESlateVisibility::Collapsed);
        UI_BACKGROUND_LevelUp_Ult->SetVisibility(ESlateVisibility::Collapsed);
    }

    // 툴팁 init
    if (IsValid(TooltipClass) && !TooltipInstance)
    {
		TooltipInstance = Cast<UUI_ToolTip>(CreateWidget<UUserWidget>(GetWorld(), TooltipClass));
        TooltipInstance->SetVisibility(ESlateVisibility::Collapsed);
        TooltipInstance->AddToViewport(10); // UI 가시성 우선순위 위로
    }
    if (!TooltipManager)
    {
        TooltipManager = NewObject<UUI_ToolTipManager>(this);
		TooltipManager->setTooltipInstance(TooltipInstance);
    }

    // 툴팁, 클릭 이벤트 바인딩
    if (skill_01)
    {
        skill_01->OnHovered.AddDynamic(this, &UUI_MainHUD::OnSkill01Hovered);
        skill_01->OnUnhovered.AddDynamic(this, &UUI_MainHUD::HideTooltip);
        skill_01->OnClicked.AddDynamic(this, &UUI_MainHUD::OnSkillClicked_Q);
    }
    if (skill_02)
    {
        skill_02->OnHovered.AddDynamic(this, &UUI_MainHUD::OnSkill02Hovered);
        skill_02->OnUnhovered.AddDynamic(this, &UUI_MainHUD::HideTooltip);
        skill_02->OnClicked.AddDynamic(this, &UUI_MainHUD::OnSkillClicked_W);
    }
    if (skill_03)
    {
        skill_03->OnHovered.AddDynamic(this, &UUI_MainHUD::OnSkill03Hovered);
        skill_03->OnUnhovered.AddDynamic(this, &UUI_MainHUD::HideTooltip);
        skill_03->OnClicked.AddDynamic(this, &UUI_MainHUD::OnSkillClicked_E);
    }
    if (skill_04)
    {
        skill_04->OnHovered.AddDynamic(this, &UUI_MainHUD::OnSkill04Hovered);
        skill_04->OnUnhovered.AddDynamic(this, &UUI_MainHUD::HideTooltip);
        skill_04->OnClicked.AddDynamic(this, &UUI_MainHUD::OnSkillClicked_R);
    }

    if (skill_up_01)
    {
        skill_up_01->OnHovered.AddDynamic(this, &UUI_MainHUD::OnSkillLevelUp01Hovered);
        skill_up_01->OnUnhovered.AddDynamic(this, &UUI_MainHUD::HideTooltip);
        skill_up_01->OnClicked.AddDynamic(this, &UUI_MainHUD::OnSkillLevelUpClicked_Q);

    }
    if (skill_up_02)
    {
        skill_up_02->OnHovered.AddDynamic(this, &UUI_MainHUD::OnSkillLevelUp02Hovered);
        skill_up_02->OnUnhovered.AddDynamic(this, &UUI_MainHUD::HideTooltip);
        skill_up_02->OnClicked.AddDynamic(this, &UUI_MainHUD::OnSkillLevelUpClicked_W);
    }
    if (skill_03)
    {
        skill_up_03->OnHovered.AddDynamic(this, &UUI_MainHUD::OnSkillLevelUp03Hovered);
        skill_up_03->OnUnhovered.AddDynamic(this, &UUI_MainHUD::HideTooltip);
        skill_up_03->OnClicked.AddDynamic(this, &UUI_MainHUD::OnSkillLevelUpClicked_E);
    }
    if (skill_04)
    {
        skill_up_04->OnHovered.AddDynamic(this, &UUI_MainHUD::OnSkillLevelUp04Hovered);
        skill_up_04->OnUnhovered.AddDynamic(this, &UUI_MainHUD::HideTooltip);
        skill_up_04->OnClicked.AddDynamic(this, &UUI_MainHUD::OnSkillLevelUpClicked_R);
    }
    // skil

    // cool
    SkillCoolTexts[0] = skill_cool_01;
    SkillCoolTexts[1] = skill_cool_02;
    SkillCoolTexts[2] = skill_cool_03;
    SkillCoolTexts[3] = skill_cool_04;

    skill_cool_01->SetVisibility(ESlateVisibility::HitTestInvisible);
    skill_cool_02->SetVisibility(ESlateVisibility::HitTestInvisible);
    skill_cool_03->SetVisibility(ESlateVisibility::HitTestInvisible);
    skill_cool_04->SetVisibility(ESlateVisibility::HitTestInvisible);
    
    // 남은 시간 초기화
    for (int32 i = 0; i < 4; i++)
    {
        RemainingTimes[i] = 0.f;
    }

    // UI 애니메이션 강제 바인딩
    HeadHitAnim_01 = GetWidgetAnimationByName(TEXT("AN_HeadHitAnim_01"));
    HeadHitAnim_02 = GetWidgetAnimationByName(TEXT("AN_HeadHitAnim_02"));

    // 페이즈 And Time
    GetWorld()->GetTimerManager().SetTimer(
        PhaseAndTimeTimer,
        this,
        &UUI_MainHUD::UpdatePhaseAndTimeText,
        1.0f,
        true);

    // 팀원 UI 기본값 숨겨놓기
    TeamHead_01->SetVisibility(ESlateVisibility::Collapsed);
    TeamHead_02->SetVisibility(ESlateVisibility::Collapsed);
    PB_TeamHP_01->SetVisibility(ESlateVisibility::Collapsed);
    PB_TeamHP_02->SetVisibility(ESlateVisibility::Collapsed);
    TeamLevel_01->SetVisibility(ESlateVisibility::Collapsed);
    TeamLevel_02->SetVisibility(ESlateVisibility::Collapsed);

    //// 디버그용
    //SetKillCount(0);
    //SetDeathCount(41);
    //SetAssistCount(411);

    //GetWorld()->GetTimerManager().SetTimer(
    //    KillTimerHandle,
    //    this,
    //    &UUI_MainHUD::AddKillPerSecond,
    //    1.0f,
    //    true);
    
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
    if (SkillDataAssets.Num() < 1) return;

    if (IsValid(SkillDataAssets[0]))
    {
        FSkillTooltipData nowSkill = SkillDataAssets[0]->GetSkillTooltipData(getSkillLevel(Q_SkillTag, false));
        ShowTooltip(skill_01, nowSkill.SkillName, nowSkill.ShortDescription, nowSkill.DetailedDescription, nowSkill.CostDescription, true);
    }


    //int32 Index = 0;
    //if (HeroData && HeroData->SkillDataAsset.IsValidIndex(Index))
    //{
    //    USkillDataAsset* SkillAsset = HeroData->SkillDataAsset[Index].LoadSynchronous();
    //    if (IsValid(SkillAsset))
    //    {
    //        FSkillTooltipData nowSkill = SkillAsset->GetSkillTooltipData(1.0f);
    //        
    //        ShowTooltip(skill_01, nowSkill.SKillIcon, nowSkill.SkillName, nowSkill.ShortDescription, nowSkill.DetailedDescription, nowSkill.CostDescription, true);
    //    }
    //}

    // ShowTooltip(skill_01, TEX_TempIcon, FText::FromString(TEXT("파이어볼")), FText::FromString(TEXT("화염 구체를 발사합니다.")), FText::FromString(TEXT("대미지: 100\n마나 소모: 50")), true);
}

void UUI_MainHUD::OnSkill02Hovered()
{
    if (SkillDataAssets.Num() < 2) return;

    if (IsValid(SkillDataAssets[1]))
    {
        FSkillTooltipData nowSkill = SkillDataAssets[1]->GetSkillTooltipData(getSkillLevel(Q_SkillTag, false));
        ShowTooltip(skill_02, nowSkill.SkillName, nowSkill.ShortDescription, nowSkill.DetailedDescription, nowSkill.CostDescription, true);
    }
}

void UUI_MainHUD::OnSkill03Hovered()
{
    if (SkillDataAssets.Num() < 3) return;

    if (IsValid(SkillDataAssets[2]))
    {
        FSkillTooltipData nowSkill = SkillDataAssets[2]->GetSkillTooltipData(getSkillLevel(Q_SkillTag, false));
        ShowTooltip(skill_03, nowSkill.SkillName, nowSkill.ShortDescription, nowSkill.DetailedDescription, nowSkill.CostDescription, true);
    }
}

void UUI_MainHUD::OnSkill04Hovered()
{
    if (SkillDataAssets.Num() < 4) return;
    
    if (IsValid(SkillDataAssets[3]))
    {
        if (ASC)
        {
            FSkillTooltipData nowSkill = SkillDataAssets[3]->GetSkillTooltipData(getSkillLevel(Q_SkillTag, false));
            ShowTooltip(skill_04, nowSkill.SkillName, nowSkill.ShortDescription, nowSkill.DetailedDescription, nowSkill.CostDescription, true);
        }
    }
}

void UUI_MainHUD::OnSkillLevelUp01Hovered()
{
}

void UUI_MainHUD::OnSkillLevelUp02Hovered()
{
}

void UUI_MainHUD::OnSkillLevelUp03Hovered()
{
}

void UUI_MainHUD::OnSkillLevelUp04Hovered()
{
}

void UUI_MainHUD::ShowTooltip(UWidget* AnchorWidget, FText Name, FText ShortDesc, FText DetailDesc, FText CostDesc, bool showUpper)
{
    if (TooltipManager)
    {
		TooltipManager->ShowTooltip(AnchorWidget, Name, ShortDesc, DetailDesc, CostDesc, showUpper);
    }


  //  if (!TooltipInstance || !AnchorWidget) return;

  //  TooltipInstance->UpdateTooltip(Icon, Name, ShortDesc, DetailDesc);
  //  TooltipInstance->SetVisibility(ESlateVisibility::HitTestInvisible);

  //  // 위젯의 위치 쓰던 말던 일단 가져오기
  //  FGeometry WidgetGeom = AnchorWidget->GetCachedGeometry();
  //  FVector2D PixelPos, ViewportPos, FinalPos;
  //  FVector2D ButtonSize = AnchorWidget->GetDesiredSize();

  //  // 버튼의 왼쪽 상단 0, 0의 절대 좌표 가져오기
  //  USlateBlueprintLibrary::LocalToViewport(GetWorld(), WidgetGeom, FVector2D(0, 0), PixelPos, ViewportPos);

  //  // 툴 팁 크기
  //  TooltipInstance->ForceLayoutPrepass();
  //  FVector2D DesiredSize = TooltipInstance->GetDesiredSize();
  //  
  //  /// 가변 해상도를 고려한 크기 계산을 위해 반드시 DPI 스케일을 곱해줘야 한다!!!!!!!!!!!!!!!!!!!
  //  float DPIScale = UWidgetLayoutLibrary::GetViewportScale(TooltipInstance);
  //  FVector2D ActualSize = DesiredSize * DPIScale;

  //  //UE_LOG(LogTemp, Error, TEXT("PixelPos : %f, %f"), PixelPos.X, PixelPos.Y);
  //  //UE_LOG(LogTemp, Error, TEXT("ViewportPos : %f, %f"), ViewportPos.X, ViewportPos.Y);
  //  //UE_LOG(LogTemp, Error, TEXT("ActualSize Size : %f, %f"), ActualSize.X, ActualSize.Y);
  //  //UE_LOG(LogTemp, Error, TEXT("ButtonSize Size : %f, %f"), ButtonSize.X, ButtonSize.Y);
  //  
  //  FinalPos.X = PixelPos.X - (ActualSize.X / 2) + (ButtonSize.X / 2);
  //  if(showUpper)
  //      FinalPos.Y = PixelPos.Y - ActualSize.Y;
  //  else
		//FinalPos.Y = PixelPos.Y + (ButtonSize.Y * 2);

  //  /// 툴팁이 밖으로 나갈 경우 안으로 들여보내기
  //  FVector2D ViewportSize;
  //  GEngine->GameViewport->GetViewportSize(ViewportSize);
  //  // 좌측 보정
  //  if (FinalPos.X + ActualSize.X > ViewportSize.X)
  //  {
  //      FinalPos.X = ViewportSize.X - ActualSize.X;
  //  }
  //  // 우측 보정
  //  if (FinalPos.X < 0.f)
  //  {
  //      FinalPos.X = 0.f;
  //  }

  //  // 상단 보정
  //  if (FinalPos.Y < 0.f)
  //  {
  //      // FinalPos.Y = 0.f;
  //      FinalPos.Y = PixelPos.Y + (ButtonSize.Y * 2);
  //  }
  //  // 하단 보정
  //  if (FinalPos.Y + ActualSize.Y > ViewportSize.Y)
  //  {
  //      // FinalPos.Y = ViewportSize.Y - ActualSize.Y;
  //      FinalPos.Y = PixelPos.Y - ActualSize.Y;
  //  }
  //  // 상단 하단의 경우 넘어서면 그냥 upper / lower를 토글하는 방식으로 처리하는게 이쁜것 갓다??
  //  // 주석 코드는 단순히 툴팁 크기만큼만 안 빠져 나가게 한다.

  //  TooltipInstance->SetPositionInViewport(FinalPos);
}

void UUI_MainHUD::HideTooltip()
{
    if (IsValid(TooltipInstance))
        TooltipInstance->SetVisibility(ESlateVisibility::Collapsed);
}

void UUI_MainHUD::initSkillDataAssets()
{
    SkillDataAssets.Empty();
    SkillDataAssets.Init(nullptr, 4);

    for (const auto& SkillAsset : HeroData->SkillDataAsset)
    {
        if (SkillAsset.IsValid() && SkillAsset->SkillConfig)
        {
            FName TagName = SkillAsset->SkillConfig->Data.InputKeyTag.GetTagName();

            if (TagName == Q_SkillTag.ToString()) SkillDataAssets[0] = SkillAsset.Get();
            else if (TagName == W_SkillTag.ToString()) SkillDataAssets[1] = SkillAsset.Get();
            else if (TagName == E_SkillTag.ToString()) SkillDataAssets[2] = SkillAsset.Get();
            else if (TagName == R_SkillTag.ToString()) SkillDataAssets[3] = SkillAsset.Get();
        }
    }
}

void UUI_MainHUD::HandleMinimapClicked(const FPointerEvent& InMouseEvent)
{
    if (!IsValid(TEX_Minimap) || !IsValid(MinimapCaptureComponent)) return;

    FGeometry MapGeometry = TEX_Minimap->GetCachedGeometry();
    FVector2D LocalClickPos = MapGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
    FVector2D ImageSize = MapGeometry.GetLocalSize();

    float AlphaX = LocalClickPos.X / ImageSize.X;
    float AlphaY = LocalClickPos.Y / ImageSize.Y;

    float OffsetXRatio = AlphaX - 0.5f;
    float OffsetYRatio = AlphaY - 0.5f;

    float minimapGap = 45.0f;
    float RotationRad = FMath::DegreesToRadians(minimapGap);

    float RotatedOffsetX = OffsetXRatio * FMath::Cos(RotationRad) - OffsetYRatio * FMath::Sin(RotationRad);
    float RotatedOffsetY = OffsetXRatio * FMath::Sin(RotationRad) + OffsetYRatio * FMath::Cos(RotationRad);


    float MapWidth = MinimapCaptureComponent->OrthoWidth;

    float RelativeWorldX = -(RotatedOffsetY * MapWidth);
    float RelativeWorldY = (RotatedOffsetX * MapWidth);

    FVector CameraLoc = MinimapCaptureComponent->GetComponentLocation();
    // 최종 목적지 계산
    FVector TargetWorldPos = FVector(CameraLoc.X + RelativeWorldX, CameraLoc.Y + RelativeWorldY, CameraLoc.Z);

    // UE_LOG(LogTemp, Log, TEXT("최종 목적지 월드 좌표: %s"), *TargetWorldPos.ToString());
    
    // 실제 이동 명령
    ABasePlayerController* PC = Cast<ABasePlayerController>(GetOwningPlayer());
    if (IsValid(PC))
    {
		// UE_LOG(LogTemp, Log, TEXT("PC에게 이동 명령 전달: %s"), *TargetWorldPos.ToString());

        FNavLocation ProjectedLocation;
        
        // -250~250 범위에 NavMesh가 있는지 찾아보기
        FVector QueryExtent(0.f, 0.f, 500.f);
		FVector ZeroExtent(0.f, 0.f, -250.f);
        UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
        if (NavSys->ProjectPointToNavigation(ZeroExtent, ProjectedLocation, QueryExtent))
        {
            float MeshZ = ProjectedLocation.Location.Z;
            // UE_LOG(LogTemp, Warning, TEXT("내 발밑 NavMesh의 Z값: %f"), MeshZ);
			TargetWorldPos.Z = MeshZ;
        }

        PC->OnMinimapClicked(TargetWorldPos);
    }

    //// 캐릭터로부터의 방향과 거리 구하기 <- 생각해보니 이거 왜 계산했지???
    //APawn* PlayerPawn = GetOwningPlayerPawn();
    //if (IsValid(PlayerPawn))
    //{
    //    FVector CharLoc = PlayerPawn->GetActorLocation();

    //    FVector DirToTarget = TargetWorldPos - CharLoc;
    //    DirToTarget.Z = 0.f; // 평면 거리만

    //    float Distance = DirToTarget.Size();
    //    FVector Direction = DirToTarget.GetSafeNormal();

    //    UE_LOG(LogTemp, Log, TEXT("캐릭터로부터 거리: %f, 방향: %s"), Distance, *Direction.ToString());
    //}
}

void UUI_MainHUD::OnSkillClicked_Q()
{
    SkillFirePressed(ESkillKey::Q);
}

void UUI_MainHUD::OnSkillReleased_Q()
{
    SkillFireReleased(ESkillKey::Q);
}

void UUI_MainHUD::OnSkillClicked_W()
{
    SkillFirePressed(ESkillKey::W);
}

void UUI_MainHUD::OnSkillReleased_W()
{
    SkillFireReleased(ESkillKey::W);
}

void UUI_MainHUD::OnSkillClicked_E()
{
    SkillFirePressed(ESkillKey::E);
}

void UUI_MainHUD::OnSkillReleased_E()
{
    SkillFireReleased(ESkillKey::E);
}

void UUI_MainHUD::OnSkillClicked_R()
{
    SkillFirePressed(ESkillKey::R);
}

void UUI_MainHUD::OnSkillReleased_R()
{
    SkillFireReleased(ESkillKey::R);
}

void UUI_MainHUD::SkillFirePressed(ESkillKey _Index)
{   
    if (!ASC) return;
	int32 EnumIndex = static_cast<int32>(_Index);

    if (SkillDataAssets.Num() < EnumIndex+1) return;

    if (IsValid(SkillDataAssets[EnumIndex]))
    {
        if (SkillDataAssets[EnumIndex]->SkillConfig)
        {
            FGameplayTag InputTag = SkillDataAssets[EnumIndex]->SkillConfig->Data.InputKeyTag;
            ABasePlayerController* PC = Cast<ABasePlayerController>(GetOwningPlayer());

            if (IsValid(PC))
            {
                PC->AbilityInputTagPressed(InputTag);

                float CoolTime = SkillDataAssets[EnumIndex]->SkillConfig->Data.BaseCoolTime.GetValueAtLevel(getSkillLevel(InputTag, false));
            }
        }
    }


 //   int32 Index = static_cast<int32>(_Index);
	//// UE_LOG(LogTemp, Warning, TEXT("SkillFirePressed called with index: %d"), Index);

 //   if (HeroData && HeroData->SkillDataAsset.IsValidIndex(Index))
 //   {
 //       USkillDataAsset* SkillAsset = HeroData->SkillDataAsset[Index].LoadSynchronous();
	//	// UE_LOG(LogTemp, Warning, TEXT("Loaded SkillDataAsset for index %d: %s"), Index, *GetNameSafe(SkillAsset));
 //       if (SkillAsset && SkillAsset->SkillConfig)
 //       {
	//		// UE_LOG(LogTemp, Warning, TEXT("SkillConfig found for index %d: %s"), Index, *GetNameSafe(SkillAsset->SkillConfig));
 //           FGameplayTag InputTag = SkillAsset->SkillConfig->Data.InputKeyTag;
 //           ABasePlayerController* PC = Cast<ABasePlayerController>(GetOwningPlayer());

 //           if (IsValid(PC))
 //           {
	//			PC->AbilityInputTagPressed(InputTag);
	//			float CoolTime = SkillAsset->SkillConfig->Data.BaseCoolTime.GetValueAtLevel(1);
	//			// UE_LOG(LogTemp, Warning, TEXT("%d_Skill, TAG : %s, CoolTime : %f"), 0, *InputTag.ToString(), CoolTime);

 //           }
 //       }
 //   }
 //   else
 //   {
	//	UE_LOG(LogTemp, Warning, TEXT("Invalid SkillDataAsset or index out of range"));
 //   }

    ///
}

void UUI_MainHUD::SkillFireReleased(ESkillKey _Index)
{
    // if (!ASC) return;

    int32 Index = static_cast<int32>(_Index);

    if (HeroData && HeroData->SkillDataAsset.IsValidIndex(Index))
    {
        USkillDataAsset* SkillAsset = HeroData->SkillDataAsset[Index].LoadSynchronous();

        if (SkillAsset && SkillAsset->SkillConfig)
        {
            FGameplayTag InputTag = SkillAsset->SkillConfig->Data.InputKeyTag;
            ABasePlayerController* PC = Cast<ABasePlayerController>(GetOwningPlayer());

            if (IsValid(PC))
            {
                PC->AbilityInputTagReleased(InputTag);
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid SkillDataAsset or index out of range"));
    }
}

void UUI_MainHUD::OnSkillLevelUpClicked_Q()
{
    //AER_PlayerState* PS = Cast<AER_PlayerState>(GetOwningPlayerState());
    //if (PS)
    //{
    //    UBaseAttributeSet* AS = nullptr;
    //    AS = PS->GetAttributeSet();
    //    AS->SetSkillPoint(AS->GetSkillPoint() - 1.0f);
    //} 스킬포인트 소모 처리는 서버에서
    getSkillLevel(Q_SkillTag, true);
}

void UUI_MainHUD::OnSkillLevelUpReleased_Q()
{
}

void UUI_MainHUD::OnSkillLevelUpClicked_W()
{
    getSkillLevel(W_SkillTag, true);
}

void UUI_MainHUD::OnSkillLevelUpReleased_W()
{
}

void UUI_MainHUD::OnSkillLevelUpClicked_E()
{
    getSkillLevel(E_SkillTag, true);
}

void UUI_MainHUD::OnSkillLevelUpReleased_E()
{
}

void UUI_MainHUD::OnSkillLevelUpClicked_R()
{
    getSkillLevel(R_SkillTag, true);
}

void UUI_MainHUD::OnSkillLevelUpReleased_R()
{
}

void UUI_MainHUD::OnAbilityActivated(UGameplayAbility* ActivatedAbility)
{
    if (!ActivatedAbility) return;

    // 현재 실행 중인 어빌리티의 Handle을 통해 Spec을 찾아오기
    FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(ActivatedAbility->GetCurrentAbilitySpecHandle());
    if (Spec)
    {
        for (const FGameplayTag& Tag : Spec->DynamicAbilityTags)
        {
            // UE_LOG(LogTemp, Error, TEXT("Spec 보유 태그: %s"), *Tag.ToString());
            
            // 좀 더 스마트한 방법이 없을지 더 찾아보자...
            if (Tag == Q_SkillTag)
            {
                OnActivateSkillCoolTime(ESkillKey::Q);
            }
            else if (Tag == W_SkillTag)
            {
                OnActivateSkillCoolTime(ESkillKey::W);
            }
            else if (Tag == E_SkillTag)
            {
                OnActivateSkillCoolTime(ESkillKey::E);
            }
            else if (Tag == R_SkillTag)
            {
                OnActivateSkillCoolTime(ESkillKey::R);
            }
        }
    }
}

void UUI_MainHUD::OnActivateSkillCoolTime(ESkillKey Skill_Index)
{
    int32 Index = static_cast<int32>(Skill_Index);
	// UE_LOG(LogTemp, Error, TEXT("스킬 %d 사용됨, 쿨타임 시작"), Index);
    // 인덱스 범위 체크 (Q~R)
    if (!SkillCoolTexts[Index] || Index < 0 || Index >= 4) return;

    if (HeroData && HeroData->SkillDataAsset.IsValidIndex(Index))
    {
        USkillDataAsset* SkillAsset = HeroData->SkillDataAsset[Index].LoadSynchronous();
        if (SkillAsset && SkillAsset->SkillConfig)
        {
            // ************************************************************************
            // 스킬 레벨을 알아올 방법을 몰라서 일단 스킬레벨 1로 처리 차후 수정해야 함
            // ************************************************************************

            // 스킬레벨 알아와서 쿨타임 적용함
			FGameplayTag InputTag = SkillAsset->SkillConfig->Data.InputKeyTag;
            float baseCool = SkillAsset->SkillConfig->Data.BaseCoolTime.GetValueAtLevel(getSkillLevel(InputTag, false));
            float finalCool = baseCool * (1.0f + (nowSkillCoolReduc / 100.0f));

            // 최종 쿨
            RemainingTimes[Index] = finalCool;

            // 타이머 시작
            GetWorld()->GetTimerManager().ClearTimer(SkillTimerHandles[Index]);
            GetWorld()->GetTimerManager().SetTimer(
                SkillTimerHandles[Index],
                [this, Index]() { UpdateSkillCoolDown(Index); },
                0.1f,
                true
            );

            // UE_LOG(LogTemp, Log, TEXT("Skill %d Timer Started: %f"), Index, finalCool);
        }
    }
}

void UUI_MainHUD::UpdateSkillCoolDown(int32 SkillIndex)
{
    RemainingTimes[SkillIndex] -= 0.1f;
    
    // 종료 처리
    if (RemainingTimes[SkillIndex] <= 0.0f)
    {
        
        GetWorld()->GetTimerManager().ClearTimer(SkillTimerHandles[SkillIndex]);
        if (SkillCoolTexts[SkillIndex])
        {
            SkillCoolTexts[SkillIndex]->SetText(FText::GetEmpty());
        }
    }
    else
    {
        if (SkillCoolTexts[SkillIndex])
        {
            FNumberFormattingOptions Opts;
            Opts.MinimumFractionalDigits = 1;
            Opts.MaximumFractionalDigits = 1;

            SkillCoolTexts[SkillIndex]->SetText(FText::AsNumber(RemainingTimes[SkillIndex], &Opts));
        }
    }
}

TArray<int32> UUI_MainHUD::GetDigitsFromNumber(int32 InNumber)
{
    TArray<int32> Digits;

    // 0예외 처리
    if (InNumber == 0)
    {
        Digits.Add(0);
        return Digits;
    }

    // 음수예외 처리
    int32 TempNumber = FMath::Abs(InNumber);

    while (TempNumber > 0)
    {
        Digits.Add(TempNumber % 10);
        TempNumber /= 10;
    }

    // 현재 Digits에는 역순으로 들어가 있어서 반전
    Algo::Reverse(Digits);

    return Digits;
}

void UUI_MainHUD::SetKillCount(int32 InKillCount)
{
    // 두자리수 고정
	// if (InKillCount > 99) InKillCount = 99;
	// TArray<int32> Digits = GetDigitsFromNumber(InKillCount);
    //

    int32 ClampedCount = FMath::Clamp(InKillCount, 0, 99);
    int32 TenDigit = ClampedCount / 10;
    int32 OneDigit = ClampedCount % 10;

    if (SegmentTextures.Num() < 10)
    {
        UE_LOG(LogTemp, Warning, TEXT("SEVEN SEGEMENT LOADING FAIL"));
        return;
    }
    if (KillNumber_01 && SegmentTextures[TenDigit])
    {
        KillNumber_01->SetBrushFromTexture(SegmentTextures[TenDigit]);
    }

    if (KillNumber_02 && SegmentTextures[OneDigit])
    {
        KillNumber_02->SetBrushFromTexture(SegmentTextures[OneDigit]);
    }

}

void UUI_MainHUD::SetDeathCount(int32 InDeathCount)
{
    // 두자리수 고정
    int32 ClampedCount = FMath::Clamp(InDeathCount, 0, 99);
    int32 TenDigit = ClampedCount / 10;
    int32 OneDigit = ClampedCount % 10;

    if (SegmentTextures.Num() < 10)
    {
        UE_LOG(LogTemp, Warning, TEXT("SEVEN SEGEMENT LOADING FAIL"));
        return;
    }
    if (DeathNumber_01 && SegmentTextures[TenDigit])
    {
        DeathNumber_01->SetBrushFromTexture(SegmentTextures[TenDigit]);
    }

    if (DeathNumber_02 && SegmentTextures[OneDigit])
    {
        DeathNumber_02->SetBrushFromTexture(SegmentTextures[OneDigit]);
    }
}

void UUI_MainHUD::SetAssistCount(int32 InAssistCount)
{
    // 두자리수 고정
    int32 ClampedCount = FMath::Clamp(InAssistCount, 0, 99);
    int32 TenDigit = ClampedCount / 10;
    int32 OneDigit = ClampedCount % 10;

    if (SegmentTextures.Num() < 10)
    {
        UE_LOG(LogTemp, Warning, TEXT("SEVEN SEGEMENT LOADING FAIL"));
        return;
    }
    if (AssistNumber_01 && SegmentTextures[TenDigit])
    {
        AssistNumber_01->SetBrushFromTexture(SegmentTextures[TenDigit]);
    }

    if (AssistNumber_02 && SegmentTextures[OneDigit])
    {
        AssistNumber_02->SetBrushFromTexture(SegmentTextures[OneDigit]);
    }
}

void UUI_MainHUD::UpdatePhaseAndTimeText()
{
    // 1. GameState 가져오기

    if (GS)
    {
        /// 시간 처리
        float RemainTime = GS->GetPhaseRemainingTime();
		// UE_LOG(LogTemp, Error, TEXT("Remain Time : %f"), RemainTime);

        int32 TotalIntSeconds = FMath::Max(0, FMath::FloorToInt(RemainTime));

        int32 Minutes = TotalIntSeconds / 60;
        int32 Seconds = TotalIntSeconds % 60;

        FString TimeString = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);

        int32 MinTenDigit = Minutes / 10;
        int32 MinOneDigit = Minutes % 10;

        int32 SecTenDigit = Seconds / 10;
        int32 SecOneDigit = Seconds % 10;

        if (PhaseTimerMinTen && SegmentTextures[MinTenDigit])
        {
            PhaseTimerMinTen->SetBrushFromTexture(SegmentTextures[MinTenDigit]);
        }
        if (PhaseTimerMinOne && SegmentTextures[MinOneDigit])
        {
            PhaseTimerMinOne->SetBrushFromTexture(SegmentTextures[MinOneDigit]);
        }
        if (PhaseTimerSecTen && SegmentTextures[SecTenDigit])
        {
            PhaseTimerSecTen->SetBrushFromTexture(SegmentTextures[SecTenDigit]);
        }
        if (PhaseTimerSecOne && SegmentTextures[SecOneDigit])
        {
            PhaseTimerSecOne->SetBrushFromTexture(SegmentTextures[SecOneDigit]);
        }

        // 페이즈 처리
        int32 nowPhase = GS->GetCurrentPhase();
        if (nowPhase > 9) nowPhase = 9;
        if (NowCurrentPhase && SegmentTextures[nowPhase])
        {
            NowCurrentPhase->SetBrushFromTexture(SegmentTextures[nowPhase]);
        }
    }
    else
    {
        /// GameState 가져오기
        GS = GetWorld()->GetGameState<AER_GameState>();
    }

}

void UUI_MainHUD::SetTeamWidgetVisible(int32 TeamIndex, bool bIsVisible)
{
    ESlateVisibility NewVisibility = bIsVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;

    if (TeamIndex == 0)
    {
        if (TeamHead_01) TeamHead_01->SetVisibility(NewVisibility);
        if (PB_TeamHP_01) PB_TeamHP_01->SetVisibility(NewVisibility);
        if (TeamLevel_01) TeamLevel_01->SetVisibility(NewVisibility);
    }
    else if (TeamIndex == 1)
    {
        if (TeamHead_02) TeamHead_02->SetVisibility(NewVisibility);
        if (PB_TeamHP_02) PB_TeamHP_02->SetVisibility(NewVisibility);
        if (TeamLevel_02) TeamLevel_02->SetVisibility(NewVisibility);
    }
}

void UUI_MainHUD::SetTeamMemberData(int32 TeamIndex, UAbilitySystemComponent* _ASC, UTexture2D* HeadIcon)
{
    if (_ASC == nullptr) return;
    TeamASCMap.Add(TeamIndex, _ASC);
    UpdateTeamHead(TeamIndex, HeadIcon);
}

void UUI_MainHUD::InitTeamData()
{
    for (auto& Pair : TeamASCMap)
    {
        int32 TeamIndex = Pair.Key;
        UAbilitySystemComponent* TargetASC = Pair.Value;

        if (IsValid(TargetASC))
        {
            TargetASC->GetGameplayAttributeValueChangeDelegate(UBaseAttributeSet::GetHealthAttribute())
                .AddUObject(this, &UUI_MainHUD::OnTeamHealthChanged, TeamIndex);
            
            TargetASC->GetGameplayAttributeValueChangeDelegate(UBaseAttributeSet::GetMaxHealthAttribute())
                .AddUObject(this, &UUI_MainHUD::OnTeamHealthChanged, TeamIndex);

            TargetASC->GetGameplayAttributeValueChangeDelegate(UBaseAttributeSet::GetLevelAttribute())
                .AddUObject(this, &UUI_MainHUD::OnTeamLevelChanged, TeamIndex);

			// 초기 HP, LV 세팅
            const UBaseAttributeSet* BaseAS = TargetASC->GetSet<UBaseAttributeSet>();
            if (BaseAS)
            {
                UpdateTeamHP(TeamIndex, BaseAS->GetHealth(), BaseAS->GetMaxHealth());
				UpdateTeamLV(TeamIndex, BaseAS->GetLevel());
            }
        }
    }
}

void UUI_MainHUD::SetMyFaceIcon(UTexture2D* HeadIcon)
{
    if (IsValid(IMG_Head))
    {
        IMG_Head->SetBrushFromTexture(HeadIcon);
    }
}

void UUI_MainHUD::OnTeamHealthChanged(const FOnAttributeChangeData& Data, int32 TeamIndex)
{
    float CurrentHP = Data.NewValue;
    float MaxHP = TeamASCMap[TeamIndex]->GetNumericAttribute(UBaseAttributeSet::GetMaxHealthAttribute());

    UpdateTeamHP(TeamIndex, CurrentHP, MaxHP);
}

void UUI_MainHUD::OnTeamLevelChanged(const FOnAttributeChangeData& Data, int32 TeamIndex)
{
    UpdateTeamLV(TeamIndex, Data.NewValue);
}

void UUI_MainHUD::AddKillPerSecond()
{
	SetKillCount(CurrentKillCount++);
    
    debugHP_01 -= 100.f;
    debugHP_02 -= 200.f;
    LastHP_01 = 1000.f;
    LastHP_02 = 1000.f;
	UpdateTeamHP(0, debugHP_01, 1000.f);
    UpdateTeamHP(1, debugHP_02, 1000.f);

    // 리스폰 타임 테스트용
	AER_PlayerState* PS = Cast<AER_PlayerState>(GetOwningPlayerState());
    float a = PS->RespawnTime;
	// UE_LOG(LogTemp, Error, TEXT("RespawnTime : %f"), a);

    if (test)
    {
        test = false;
        UBaseAttributeSet* AS = nullptr;
        AS = PS->GetAttributeSet();
        AS->SetSkillPoint(AS->GetSkillPoint() + 1.0f);
    }
}

void UUI_MainHUD::WarningSign(int number)
{
    int32 TotalIntSeconds = FMath::Clamp(number, 0, 99);
    int32 Seconds = TotalIntSeconds % 60;

    int32 SecTenDigit = Seconds / 10;
    int32 SecOneDigit = Seconds % 10;

    if (WarningNumber_ten && SegmentTextures[SecTenDigit])
    {
        WarningNumber_ten->SetBrushFromTexture(SegmentTextures[SecTenDigit]);
    }
    if (WarningNumber_one && SegmentTextures[SecOneDigit])
    {
        WarningNumber_one->SetBrushFromTexture(SegmentTextures[SecOneDigit]);
    }

}

void UUI_MainHUD::UpdateTeamHP(int32 TeamIndex, float CurrentHP, float MaxHP)
{
    if (TeamIndex > MAX_TEAMMATE) return;
    if (CurrentHP < 0) return;

    if(TeamIndex == 0)
    {
        if(IsValid(PB_TeamHP_01))
        {
            float HealthPercent = CurrentHP / MaxHP;
            PB_TeamHP_01->SetPercent(HealthPercent);

            if (CurrentHP < LastHP_01)
            {
                if (HeadHitAnim_01 && !IsAnimationPlaying(HeadHitAnim_01))
                {
                    PlayAnimation(HeadHitAnim_01);
                }
            }
            LastHP_01 = CurrentHP;
        }
    }
    else if(TeamIndex == 1)
    {
        if(IsValid(PB_TeamHP_02))
        {
            float HealthPercent = CurrentHP / MaxHP;
            PB_TeamHP_02->SetPercent(HealthPercent);

            if (CurrentHP < LastHP_02)
            {
                if (HeadHitAnim_02 && !IsAnimationPlaying(HeadHitAnim_02))
                {
                    PlayAnimation(HeadHitAnim_02);
                }
            }
            LastHP_02 = CurrentHP;
        }
	}
}


void UUI_MainHUD::UpdateTeamLV(int32 TeamIndex, int32 CurrentLV)
{
    if (TeamIndex > MAX_TEAMMATE) return;

    if(TeamIndex == 0)
    {
        if(IsValid(TeamLevel_01))
        {
			// UE_LOG(LogTemp, Error, TEXT("Updating Team 1 Level: %d"), CurrentLV);
            TeamLevel_01->SetText(FText::AsNumber(CurrentLV));
        }
    }
    else if (TeamIndex == 1)
    {
        if (IsValid(TeamLevel_02))
        {
            // UE_LOG(LogTemp, Error, TEXT("Updating Team 1 Level: %d"), CurrentLV);
            TeamLevel_02->SetText(FText::AsNumber(CurrentLV));
        }
    }
}

void UUI_MainHUD::UpdateTeamHead(int32 TeamIndex, UTexture2D* NewHeadTexture)
{
    if (TeamIndex > MAX_TEAMMATE) return;
    if(TeamIndex == 0)
    {
        if(IsValid(TeamHead_01) && NewHeadTexture)
        {
            TeamHead_01->SetBrushFromTexture(NewHeadTexture);
        }
    }
    else if (TeamIndex == 1)
    {
        if (IsValid(TeamHead_02) && NewHeadTexture)
        {
            TeamHead_02->SetBrushFromTexture(NewHeadTexture);
        }
	}
}

UWidgetAnimation* UUI_MainHUD::GetWidgetAnimationByName(FName AnimName) const
{
    UWidgetBlueprintGeneratedClass* WidgetClass = Cast<UWidgetBlueprintGeneratedClass>(GetClass());
    if (!WidgetClass) return nullptr;

    for (UWidgetAnimation* Anim : WidgetClass->Animations)
    {
        if (Anim && Anim->GetMovieScene())
        {
            FName InternalName = Anim->GetMovieScene()->GetFName();
			// UE_LOG(LogTemp, Error, TEXT("Checking Animation: %s"), *InternalName.ToString());
            if (InternalName == AnimName)
            {
                return Anim;
            }
        }
    }
    return nullptr;
}

// [김현수 추가분]
// 인벤토리 UI 업데이트 함수
void UUI_MainHUD::UpdateInventoryUI()
{
    if (!Grid_item)
    {
        return;
    }

    EnsureInventorySlotWidgets();

    UBaseInventoryComponent* InventoryComp = ResolveInventoryComponent();
    if (!InventoryComp)
    {
        return;
    }

    for (int32 i = 0; i < InventorySlotWidgets.Num(); ++i)
    {
        if (InventorySlotWidgets[i])
        {
            InventorySlotWidgets[i]->SetItemData(InventoryComp->GetItemAt(i));
            InventorySlotWidgets[i]->SetStackCount(InventoryComp->GetStackCountAt(i));
        }
    }
}

// [김현수 추가분]
UUI_MainHUD::UUI_MainHUD(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    InventoryColumnCount = 4;
    DefaultInventorySlotCount = 8;
}

UBaseInventoryComponent* UUI_MainHUD::ResolveInventoryComponent() const
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        return nullptr;
    }

    APawn* Pawn = PC->GetPawn();
    if (!Pawn)
    {
        return nullptr;
    }

    return Pawn->FindComponentByClass<UBaseInventoryComponent>();
}

void UUI_MainHUD::EnsureInventorySlotWidgets()
{
    if (!Grid_item)
    {
        return;
    }

    int32 DesiredSlotCount = DefaultInventorySlotCount;

    if (UBaseInventoryComponent* InventoryComp = ResolveInventoryComponent())
    {
        DesiredSlotCount = FMath::Max(InventoryComp->MaxSlots, 0);
    }

    if (InventorySlotWidgets.Num() == DesiredSlotCount &&
        Grid_item->GetChildrenCount() == DesiredSlotCount)
    {
        return;
    }

    Grid_item->ClearChildren();
    InventorySlotWidgets.Empty();

    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        return;
    }

    for (int32 i = 0; i < DesiredSlotCount; ++i)
    {
        UW_InventorySlot* SlotWidget = CreateWidget<UW_InventorySlot>(PC, UW_InventorySlot::StaticClass());
        
        if (!SlotWidget)
        {
            continue;
        }

        SlotWidget->SetSlotIndex(i);
        InventorySlotWidgets.Add(SlotWidget);

        if (UUniformGridSlot* GridSlot = Grid_item->AddChildToUniformGrid(SlotWidget, i / InventoryColumnCount, i % InventoryColumnCount))
        {
            GridSlot->SetHorizontalAlignment(HAlign_Fill);
            GridSlot->SetVerticalAlignment(VAlign_Fill);
        }
    }
}

// [김현수 추가분]
void UUI_MainHUD::RefreshInventoryGridLayout()
{
    if (!Grid_item)
    {
        return;
    }

    Grid_item->SetMinDesiredSlotWidth(64.f);
    Grid_item->SetMinDesiredSlotHeight(64.f);

    const TArray<UWidget*> GridChildren = Grid_item->GetAllChildren();

    for (int32 i = 0; i < GridChildren.Num(); ++i)
    {
        if (UUniformGridSlot* GridSlot = Cast<UUniformGridSlot>(GridChildren[i]->Slot))
        {
            GridSlot->SetRow(i / InventoryColumnCount);
            GridSlot->SetColumn(i % InventoryColumnCount);
            GridSlot->SetHorizontalAlignment(HAlign_Center);
            GridSlot->SetVerticalAlignment(VAlign_Center);
        }
    }
}

float UUI_MainHUD::getSkillLevel(FGameplayTag SkillTag, bool levelUp)
{
    if (levelUp)
    {

        ABasePlayerController* PC = Cast<ABasePlayerController>(GetOwningPlayer());
        if (!IsValid(PC))
        {
            return -1;
        }
        APawn* Pawn = PC->GetPawn();
        if (!IsValid(Pawn))
        {
            return -1;
        }
        ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(Pawn);
        if (!IsValid(BaseCharacter))
        {
            return -1;
        }
        BaseCharacter->Server_UpgradeSkill(SkillTag);
        return -1;
    }
    // else일 때는 단순히 스킬 레벨이 몇인지 반환한다
    else
    {
        if (!IsValid(ASC))
        {
            return -1;
        }
        UBaseAttributeSet* AS = nullptr;
        AER_PlayerState* PS = Cast<AER_PlayerState>(GetOwningPlayerState());
        {
            AS = PS->GetAttributeSet();
        }
        if (!AS)
        {
            return -1;
        }

        FGameplayAbilitySpec* TargetSpec = nullptr;
        for (FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
        {
            // SkillTag를 알아내야해
            if (!Spec.Ability) continue;

            if (Spec.GetDynamicSpecSourceTags().HasTagExact(SkillTag) ||
                Spec.Ability->GetAssetTags().HasTagExact(SkillTag))
            {
                TargetSpec = &Spec;
                break;
            }
        }
        
        return TargetSpec->Level;
    }
    return -1;
}
