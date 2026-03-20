#include "ER_OutGameMode.h"
#include "GameModeBase/State/ER_PlayerState.h"
#include "GameModeBase/State/ER_GameState.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "CharacterSystem/Player/BasePlayerController.h"
#include "GameModeBase/ER_OutGamePlayerController.h"

AER_OutGameMode::AER_OutGameMode()
{
	bUseSeamlessTravel = true;
}

void AER_OutGameMode::BeginPlay()
{
    Super::BeginPlay();

    /// mpyi _ 마우스 보이게 하기
    // 서버에선 실행 안되게
    if (UKismetSystemLibrary::IsDedicatedServer(GetWorld()))
    {
        return;
    }

    // 로컬에서만 실행
    //APlayerController* PC = GetWorld()->GetFirstPlayerController();
    //if (IsValid(PC))
    //{
    //    FInputModeUIOnly InputMode;
    //    PC->SetInputMode(InputMode);
    //    PC->bShowMouseCursor = true;
    //}
}

void AER_OutGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
    Super::HandleStartingNewPlayer_Implementation(NewPlayer);

    if (ABasePlayerController* PC = Cast<ABasePlayerController>(NewPlayer))
    {
        PC->Client_OutGameInputMode();
    }

}

FString AER_OutGameMode::InitNewPlayer(APlayerController* NewPlayerController,
    const FUniqueNetIdRepl& UniqueId,
    const FString& Options,
    const FString& Portal)
{
    Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);

    // URL에서 닉네임 파싱
    FString PlayerName = UGameplayStatics::ParseOption(Options, TEXT("PlayerName"));

    if (!PlayerName.IsEmpty())
    {
        // PlayerState에 닉네임 설정
        if (APlayerState* PS = NewPlayerController->GetPlayerState<APlayerState>())
        {
            
            if (AER_PlayerState* ERPS = Cast<AER_PlayerState>(PS))
            {
                ERPS->SetPlayerName(PlayerName);
                ERPS->SetPlayerStateName(PlayerName);

                UE_LOG(LogTemp, Warning, TEXT("InitNewPlayer : %s"), *ERPS->GetPlayerName());
                return TEXT("");
            }
        }
    }
    return TEXT("");
}

void AER_OutGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (!HasAuthority() || !NewPlayer) return;

    if (APlayerState* PS = NewPlayer->GetPlayerState<APlayerState>())
    {
        if (AER_PlayerState* ERPS = Cast<AER_PlayerState>(PS))
        {
            AER_GameState* GS = GetGameState<AER_GameState>();
            if (!GS)
            {
                return;
            }
            const TArray<APlayerState*>& Players = GS->PlayerArray;

            int32 Team1 = 0, Team2 = 0, Team3 = 0;
            for (APlayerState* it : Players)
            {
                AER_PlayerState* state = Cast<AER_PlayerState>(it);
                if (state->TeamType == ETeamType::Team_A)
                {
                    ++Team1;
                }
                else if (state->TeamType == ETeamType::Team_B)
                {
                    ++Team2;
                }
                else if (state->TeamType == ETeamType::Team_C)
                {
                    ++Team3;
                }
            }

            if (Team1 < 3)
            {
                ERPS->TeamType = ETeamType::Team_A;
            }
            else if (Team2 < 3)
            {
                ERPS->TeamType = ETeamType::Team_B;
            }
            else if (Team3 < 3)
            {
                ERPS->TeamType = ETeamType::Team_C;
            }

            OnPlayerTeamDecided(ERPS->TeamType);//Call the BP exposed Function -> use it in the editor // 2026/03/05 박세민

            UE_LOG(LogTemp, Log, TEXT("Team = %s"), *UEnum::GetValueAsString(ERPS->TeamType));
        }

    }
}

void AER_OutGameMode::StartGame()
{
    UE_LOG(LogTemp, Log, TEXT("[GM] : StartGame"));

    AER_GameState* GS = GetGameState<AER_GameState>();
    if (GS)
    {
        UE_LOG(LogTemp, Log, TEXT("[GM] : StartGame"));

        // 트래블 전 모든 클라이언트에게 로딩 화면을 띄우도록 지시
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            if (ABasePlayerController* PC = Cast<ABasePlayerController>(It->Get()))
            {
                PC->Client_OpenLoadingUI();
            }
        }

        int32 PlayerCount = GS->PlayerArray.Num();
        //FString TravelURL = FString::Printf(TEXT("/Game/Level/BattleMap/BattleMap?PlayerCount=%d"), PlayerCount);
        FString TravelURL = FString::Printf(TEXT("/Game/Level/BasicMap?PlayerCount=%d"), PlayerCount);

        GetWorld()->ServerTravel(TravelURL, true);
    }
}

void AER_OutGameMode::EndGame()
{
	if (!HasAuthority())
		return;

	GetWorld()->ServerTravel("/Game/Level/Level_Lobby", true);
}

void AER_OutGameMode::ShowCharacterSelectionToAll()
{
    UE_LOG(LogTemp, Log, TEXT("[GM] : ShowCharacterSelectionToAll"));

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (ABasePlayerController* PC = Cast<ABasePlayerController>(It->Get()))
        {
            PC->Client_ShowCharacterSelectionUI();
        }
    }
}

void AER_OutGameMode::MoveTeam(APlayerController* Player, int32 TeamIdx)
{
    if (!HasAuthority())
    {
        return;
    }

    ABasePlayerController* PC = Cast<ABasePlayerController>(Player);
    if (!PC)
    {
        return;
    }

    if (APlayerState* PS = PC->GetPlayerState<APlayerState>())
    {
        if (AER_PlayerState* ERPS = Cast<AER_PlayerState>(PS))
        {
            AER_GameState* GS = GetGameState<AER_GameState>();
            if (!GS)
            {
                return;
            }
            const TArray<APlayerState*>& Players = GS->PlayerArray;

            int32 Team1 = 0, Team2 = 0, Team3 = 0;
            for (APlayerState* it : Players)
            {
                AER_PlayerState* state = Cast<AER_PlayerState>(it);
                if (state->TeamType == ETeamType::Team_A)
                {
                    ++Team1;
                }
                else if (state->TeamType == ETeamType::Team_B)
                {
                    ++Team2;
                }
                else if (state->TeamType == ETeamType::Team_C)
                {
                    ++Team3;
                }
            }

            switch (TeamIdx)
            {
                case 1:
                    if (Team1 < 3 && ERPS->TeamType != ETeamType::Team_A)
                    {
                        ERPS->TeamType = ETeamType::Team_A;
                        OnPlayerTeamDecided(ERPS->TeamType);
                    }
                    break;

                case 2:
                    if (Team2 < 3 && ERPS->TeamType != ETeamType::Team_B)
                    {
                        ERPS->TeamType = ETeamType::Team_B;
                        OnPlayerTeamDecided(ERPS->TeamType);
                    }
                    break;

                case 3:
                    if (Team3 < 3 && ERPS->TeamType != ETeamType::Team_C)
                    {
                        ERPS->TeamType = ETeamType::Team_C;
                        OnPlayerTeamDecided(ERPS->TeamType);
                    }
                    break;

                default:

                    break;
            }

        }

    }




}
