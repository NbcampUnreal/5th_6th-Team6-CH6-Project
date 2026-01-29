#include "ER_OutGameMode.h"
#include "GameModeBase/State/ER_PlayerState.h"
#include "GameModeBase/State/ER_GameState.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"


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
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (IsValid(PC))
    {
        FInputModeUIOnly InputMode;
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = true;
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

    UE_LOG(LogTemp, Log, TEXT("[GM] : StartGame"));

    AER_GameState* GS = GetGameState<AER_GameState>();
    if (GS)
    {
        UE_LOG(LogTemp, Log, TEXT("[GM] : StartGame"));
        int32 PlayerCount = GS->PlayerArray.Num();
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
