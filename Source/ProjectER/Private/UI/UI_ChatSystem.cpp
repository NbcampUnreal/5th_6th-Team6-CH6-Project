// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/UI_ChatSystem.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Components/ScrollBoxSlot.h"
#include "GameModeBase/State/ER_PlayerState.h"
#include "CharacterSystem/Player/BasePlayerController.h"

void UUI_ChatSystem::NativeConstruct()
{
	Super::NativeConstruct();
	if (ET_ChatInput)
	{
		ET_ChatInput->OnTextCommitted.AddDynamic(this, &UUI_ChatSystem::OnChatCommitted);
	}

	if (SB_TextScrollBox)
	{
		SB_TextScrollBox->SetScrollBarVisibility(ESlateVisibility::Collapsed);
		SB_TextScrollBox->SetVisibility(ESlateVisibility::HitTestInvisible);
		SB_TextScrollBox->SetConsumeMouseWheel(EConsumeMouseWheel::Never);

		FScrollBoxStyle NewStyle = SB_TextScrollBox->GetWidgetStyle();
		NewStyle.SetTopShadowBrush(FSlateNoResource());
		NewStyle.SetBottomShadowBrush(FSlateNoResource());

		SB_TextScrollBox->SetWidgetStyle(NewStyle);

	}

	SetChatInputVisible(false);
}

void UUI_ChatSystem::OnChatCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{

	if (CommitMethod == ETextCommit::OnEnter || CommitMethod == ETextCommit::OnCleared)
	{
		APlayerController* PC = GetOwningPlayer();
		if (!PC) return;

		ABasePlayerController* BasePC = Cast<ABasePlayerController>(PC);

		if (CommitMethod == ETextCommit::OnEnter && !Text.IsEmpty())
		{
			if (BasePC)
			{
				FText PlayerName = MyPS ? FText::FromString(MyPS->GetPlayerName()) : FText::FromString("Unknown");

				FText FormattedMessage = FText::Format(FText::FromString("{0}: {1}"), PlayerName, Text);

				BasePC->Server_SendMessage(FormattedMessage.ToString());
			}
		}

		ET_ChatInput->SetText(FText::GetEmpty());
		SetChatInputVisible(false);

		if (BasePC)
		{
			BasePC->Client_InGameInputMode();
			FSlateApplication::Get().SetAllUserFocusToGameViewport();
		}
	}
}

void UUI_ChatSystem::AddMessageToScrollBox(const FString& Message)
{
	if (!SB_TextScrollBox) return;

	UTextBlock* NewMessage = NewObject<UTextBlock>(this);
	if (NewMessage)
	{
		NewMessage->SetText(FText::FromString(Message));
		NewMessage->SetColorAndOpacity(ChatColor);
		NewMessage->SetAutoWrapText(true);

		UPanelSlot* PanelSlot = SB_TextScrollBox->AddChild(NewMessage);
		UScrollBoxSlot* ScrollBoxSlot = Cast<UScrollBoxSlot>(PanelSlot);

		if (ScrollBoxSlot)
		{
			ScrollBoxSlot->SetVerticalAlignment(VAlign_Bottom);
			ScrollBoxSlot->SetPadding(FMargin(5.f, 2.f));
		}

		// 텍스트 크기 설정 추가
		FSlateFontInfo FontInfo = NewMessage->GetFont();
		FontInfo.Size = 15; // 폰트 크기
		NewMessage->SetFont(FontInfo);
		NewMessage->SetVisibility(ESlateVisibility::HitTestInvisible);

		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this, NewMessage]()
			{
				if (NewMessage && SB_TextScrollBox)
				{
					SB_TextScrollBox->RemoveChild(NewMessage);
				}
			}, 10.0f, false); // 10초뒤 채팅 삭제


		SB_TextScrollBox->AddChild(NewMessage);
		SB_TextScrollBox->ScrollToEnd();
	}
}

void UUI_ChatSystem::SetChatInputVisible(bool bVisible)
{
	if (!ET_ChatInput) return;

	if (bVisible)
	{
		ET_ChatInput->SetVisibility(ESlateVisibility::Visible);
		ET_ChatInput->SetKeyboardFocus();
	}
	else
	{
		ET_ChatInput->SetVisibility(ESlateVisibility::Hidden);
	}
}
