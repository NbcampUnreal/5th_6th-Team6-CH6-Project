// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/UI_ScoreboardPlayerRow.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"

#include "Kismet/GameplayStatics.h" // gamestate용
#include "GameModeBase/State/ER_GameState.h" // gamestate
#include "GameModeBase/State/ER_PlayerState.h"

#include "CharacterSystem/Character/BaseCharacter.h"
#include "CharacterSystem/Data/CharacterData.h"

#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"

void UUI_ScoreboardPlayerRow::Init(AER_PlayerState* PlayerState)
{
	UpdatePlayerName(FText::FromString(PlayerState->GetPlayerName()));
	UpdatePlayerKill(PlayerState->GetKillCount());
	UpdatePlayerDeath(PlayerState->GetDeathCount());
	UpdatePlayerAssist(PlayerState->GetAssistCount());

	APawn* OwningPawn = PlayerState->GetPawn();
	if (OwningPawn)
	{
		ABaseCharacter* MyChar = Cast<ABaseCharacter>(OwningPawn);
		UpdatePlayerIcon(MyChar->HeroData->CharacterIcon);
	}

	if (UAbilitySystemComponent* ASC = PlayerState->GetAbilitySystemComponent())
	{
		float CurrentLevel = ASC->GetNumericAttribute(UBaseAttributeSet::GetLevelAttribute());
		float currentHP = ASC->GetNumericAttribute(UBaseAttributeSet::GetHealthAttribute());
		float maxHP = ASC->GetNumericAttribute(UBaseAttributeSet::GetMaxHealthAttribute());
		UpdatePlayerLV(CurrentLevel);
		UpdatePlayerHP(currentHP, maxHP);
	}
	
	ETeamType TeamIndex = PlayerState->GetTeamType();
		
	FLinearColor TeamColor;
	switch (TeamIndex)
	{
	case ETeamType::Team_A:
		TeamColor = FLinearColor(0.5f, 0.1f, 0.1f, 0.8f);
		break;
	case ETeamType::Team_B:
		TeamColor = FLinearColor(0.1f, 0.1f, 0.5f, 0.8f);
		break;
	case ETeamType::Team_C:
		TeamColor = FLinearColor(0.1f, 0.5f, 1.0f, 0.8f);
		break;
	default:
		TeamColor = FLinearColor(0.2f, 0.2f, 0.2f, 0.5f);
		break;
	}
	UpdatePlayerBG(TeamColor);

}

void UUI_ScoreboardPlayerRow::UpdatePlayerName(FText PlayerName)
{
	if(txtPlayerName)
	{
		txtPlayerName->SetText(PlayerName);
	}
}

void UUI_ScoreboardPlayerRow::UpdatePlayerLV(float PlayerLV)
{
	if(txtPlayerLV)
	{
		FString LevelString = TEXT("LV ") + FString::FromInt(FMath::RoundToInt(PlayerLV));
		FText Level = FText::FromString(LevelString);
		txtPlayerLV->SetText(Level);
	}
}

void UUI_ScoreboardPlayerRow::UpdatePlayerKill(float PlayerKill)
{
	if(txtPlayerKill)
	{
		FString KillString = FString::FromInt(FMath::RoundToInt(PlayerKill)) + TEXT(" /");
		FText Kill = FText::FromString(KillString);
		
		txtPlayerKill->SetText(Kill);
	}
}

void UUI_ScoreboardPlayerRow::UpdatePlayerDeath(float PlayerDeath)
{
	if(txtPlayerDeath)
	{
		FString DeathString = FString::FromInt(FMath::RoundToInt(PlayerDeath)) + TEXT(" /");
		FText Death = FText::FromString(DeathString);

		txtPlayerDeath->SetText(Death);
	}
}

void UUI_ScoreboardPlayerRow::UpdatePlayerAssist(float PlayerAssist)
{
	if(txtPlayerAssist)
	{
		txtPlayerAssist->SetText(FText::AsNumber(FMath::RoundToInt(PlayerAssist)));
	}
}

void UUI_ScoreboardPlayerRow::UpdatePlayerHP(float CurrentHP, float MaxHP)
{
	if (PB_HP)
	{
		float HealthPercent = CurrentHP / MaxHP;
		PB_HP->SetPercent(HealthPercent);
	}
	if (txtHP)
	{
		txtHP->SetText(FText::Format(NSLOCTEXT("Scoreboard", "HPFormat", "{0} / {1}"), FText::AsNumber(FMath::RoundToInt(CurrentHP)), FText::AsNumber(FMath::RoundToInt(MaxHP))));
	}
}

void UUI_ScoreboardPlayerRow::UpdatePlayerIcon(UTexture2D* Icon)
{
	if(Img_Icon)
	{
		Img_Icon->SetBrushFromTexture(Icon);
	}
}

void UUI_ScoreboardPlayerRow::UpdatePlayerBG(FLinearColor BGColor)
{
	if (Img_BG)
	{
		Img_BG->SetColorAndOpacity(BGColor);
	}
}
