#include "ER_OutGameMode.h"
#include "ER_PlayerState.h"
#include "ER_GameState.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"

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

            // 들어온 순서대로 팀 지정 이후 팀 선택이 필요하면 수정
            switch (Players.Num() % 3)
            {
            case 1:
                ERPS->Team = ETeam::Team1;
                break;

            case 2:
                ERPS->Team = ETeam::Team2;
                break;

            case 0:
                ERPS->Team = ETeam::Team3;
                break;

            default:
                ERPS->Team = ETeam::None;
                break;
            }
            UE_LOG(LogTemp, Log, TEXT("Team = %s"), *UEnum::GetValueAsString(ERPS->Team));
        }

    }
}

void AER_OutGameMode::StartGame()
{
	if (!HasAuthority())
		return;

	GetWorld()->ServerTravel("/Game/Level/BasicMap", true);
}

void AER_OutGameMode::EndGame()
{
	if (!HasAuthority())
		return;

	GetWorld()->ServerTravel("/Game/Level/Level_Lobby", true);
}
