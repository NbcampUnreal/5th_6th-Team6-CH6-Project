// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/UI_Scoreboard.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"

#include "Kismet/GameplayStatics.h" // gamestate용
#include "GameModeBase/State/ER_GameState.h" // gamestate
#include "GameModeBase/State/ER_PlayerState.h"

#include "UI/UI_ScoreboardPlayerRow.h"

void UUI_Scoreboard::NativeConstruct()
{
    Super::NativeConstruct();
    GS = GetWorld()->GetGameState<AER_GameState>();

}


void UUI_Scoreboard::UpdateScoreboard()
{
    SB_PlayerRow->ClearChildren();

    if (GS)
    {
        for (APlayerState* PS : GS->PlayerArray)
        {
			AER_PlayerState* MyPS = Cast<AER_PlayerState>(PS);
            if (MyPS)
            {
                
                UUI_ScoreboardPlayerRow* Row = CreateWidget<UUI_ScoreboardPlayerRow>(GetWorld(), RowWidgetClass);
                if (Row)
                {
                    
                    Row->Init(MyPS);                    
                    SB_PlayerRow->AddChild(Row);
                }
            }
        }
    }
}
