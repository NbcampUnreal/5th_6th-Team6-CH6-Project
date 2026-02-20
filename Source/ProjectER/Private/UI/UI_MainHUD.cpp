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
    if (IsValid(ASC))
    {
        ASC->AbilityActivatedCallbacks.AddUObject(this, &UUI_MainHUD::OnAbilityActivated);
    }
    
}

void UUI_MainHUD::StartRespawn(float _RespawnTime)
{
	UE_LOG(LogTemp, Error, TEXT("StartRespawn called with time: %f"), _RespawnTime);
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
    // skil

    // cool
    SkillCoolTexts[0] = skill_cool_01;
    SkillCoolTexts[1] = skill_cool_02;
    SkillCoolTexts[2] = skill_cool_03;
    SkillCoolTexts[3] = skill_cool_04;
    
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

    // 디버그용
    SetKillCount(0);
    SetDeathCount(41);
    SetAssistCount(411);

    GetWorld()->GetTimerManager().SetTimer(
        KillTimerHandle,
        this,
        &UUI_MainHUD::AddKillPerSecond,
        1.0f,
        true);
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
    if (TooltipManager)
    {
		TooltipManager->ShowTooltip(AnchorWidget, Icon, Name, ShortDesc, DetailDesc, showUpper);
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
				PC->AbilityInputTagPressed(InputTag);
				float CoolTime = SkillAsset->SkillConfig->Data.BaseCoolTime.GetValueAtLevel(1);
				UE_LOG(LogTemp, Error, TEXT("%d_Skill, TAG : %s, CoolTime : %f"), 0, *InputTag.ToString(), CoolTime);

            }



            //// ASC를 통한 스킬 실행?? <- 자체제작, PC에서 가져오는걸로 퉁치는게 조을것같음.
            //if (ASC)
            //{
            //    FGameplayTagContainer TagContainer;
            //    TagContainer.AddTag(InputTag);
            //    ASC->TryActivateAbilitiesByTag(TagContainer);
            //    UE_LOG(LogTemp, Error, TEXT("%d_Skill, TAG : %s)"), 0, *InputTag.ToString());
            //}

            //////////      for (FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
            //////////      {
            //////////          if (Spec.DynamicAbilityTags.HasTagExact(InputTag))
            //////////          {
            //////////              if (Spec.IsActive())
            //////////              {
            //////////                  // [방법 2 핵심] 태그를 담은 이벤트를 어빌리티에 직접 쏩니다.
            //////////                  FGameplayEventData Payload;
            //////////                  Payload.EventTag = InputTag; // 전달할 태그
            ////////                  //ABasePlayerController* PC = Cast<ABasePlayerController>(GetOwningPlayer());
            //////////                  Payload.Instigator = PC;

            //////////                  // 활성화된 어빌리티에게 이벤트를 전달합니다.
            //////////                  ASC->HandleGameplayEvent(InputTag, &Payload);
            //////////                  UE_LOG(LogTemp, Log, TEXT("Gameplay Event Sent: %s"), *InputTag.ToString());
            //////////              }
            //////////              else
            //////////              {
            //////////                  ASC->TryActivateAbility(Spec.Handle);
            //////////              }
            //////////          }
            //////////      }
        }
    }
    else
    {
		UE_LOG(LogTemp, Warning, TEXT("Invalid SkillDataAsset or index out of range"));
    }

    ///
}

void UUI_MainHUD::SkillFireReleased(ESkillKey _Index)
{
    if (!ASC) return;

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

void UUI_MainHUD::OnAbilityActivated(UGameplayAbility* ActivatedAbility)
{
    if (!ActivatedAbility) return;

    // 현재 실행 중인 어빌리티의 Handle을 통해 Spec을 찾아오기
    FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(ActivatedAbility->GetCurrentAbilitySpecHandle());
    if (Spec)
    {
        for (const FGameplayTag& Tag : Spec->DynamicAbilityTags)
        {
            // UE_LOG(LogTemp, Log, TEXT("Spec 보유 태그: %s"), *Tag.ToString());
            
            // 좀 더 스마트한 방법이 없을지 더 찾아보자...
            if (Tag.ToString() == "Input.Action.Skill.Q")
            {
                OnActivateSkillCoolTime(ESkillKey::Q);
            }
            else if (Tag.ToString() == "Input.Action.Skill.W")
            {
                OnActivateSkillCoolTime(ESkillKey::W);
            }
            else if (Tag.ToString() == "Input.Action.Skill.E")
            {
                OnActivateSkillCoolTime(ESkillKey::E);
            }
            else if (Tag.ToString() == "Input.Action.Skill.R")
            {
                OnActivateSkillCoolTime(ESkillKey::R);
            }
        }
    }
}

void UUI_MainHUD::OnActivateSkillCoolTime(ESkillKey Skill_Index)
{
    int32 Index = static_cast<int32>(Skill_Index);

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
            float baseCool = SkillAsset->SkillConfig->Data.BaseCoolTime.GetValueAtLevel(1);
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
            TeamLevel_01->SetText(FText::AsNumber(CurrentLV));
        }
    }
    else if (TeamIndex == 1)
    {
        if (IsValid(TeamLevel_02))
        {
            TeamLevel_02->SetText(FText::AsNumber(CurrentLV));
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
			UE_LOG(LogTemp, Error, TEXT("Checking Animation: %s"), *InternalName.ToString());
            if (InternalName == AnimName)
            {
                return Anim;
            }
        }
    }
    return nullptr;
}