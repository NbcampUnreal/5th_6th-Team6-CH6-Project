#include "GameModeBase/GameMode/ER_InGameMode.h"
#include "GameModeBase/State/ER_PlayerState.h"
#include "GameModeBase/State/ER_GameState.h"
#include "GameModeBase/Subsystem/Respawn/ER_RespawnSubsystem.h"
#include "GameModeBase/Subsystem/NeutralSpawn/ER_NeutralSpawnSubsystem.h"
#include "GameModeBase/Subsystem/Phase/ER_PhaseSubsystem.h"
#include "GameModeBase/Subsystem/Object/ER_ObjectSubsystem.h"

#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameSession.h"

#include "Monster/BaseMonster.h"

#include "CharacterSystem/Player/BasePlayerController.h"

void AER_InGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (UKismetSystemLibrary::IsDedicatedServer(GetWorld()))
	{
		return;
	}


}

void AER_InGameMode::PostSeamlessTravel()
{
	Super::PostSeamlessTravel();

	UE_LOG(LogTemp, Warning, TEXT("[GM] PostSeamlessTravel - Expecting %d players"), ExpectedPlayers);
}

void AER_InGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	FString PlayerCountStr = UGameplayStatics::ParseOption(Options, TEXT("PlayerCount"));

	if (!PlayerCountStr.IsEmpty())
	{
		ExpectedPlayers = FCString::Atoi(*PlayerCountStr);
		UE_LOG(LogTemp, Warning, TEXT("[GM] InitGame - ExpectedPlayers from URL: %d"), ExpectedPlayers);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[GM] InitGame - No PlayerCount in URL Options!"));
	}
}

void AER_InGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	// 남아있는 플레이어 컨트롤러 수 계산
	int32 RemainingPlayers = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (IsValid(PC) && PC->PlayerState)
		{
			++RemainingPlayers;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[GM] Logout. RemainingPlayers=%d"), RemainingPlayers);

	if (RemainingPlayers <= 1)
	{
		//로그아웃 시점에 플레이어가 1명일 시에 서버 초기화
		EndGame();
	}
}

void AER_InGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	if (!NewPlayer)
		return;

	if (ABasePlayerController* PC = Cast<ABasePlayerController>(NewPlayer))
	{
		PC->Client_InGameInputMode();
	}
	UE_LOG(LogTemp, Warning, TEXT("[GM] HSNPlayer this=%p world=%p map=%s PI=%d/%d"),
		this, GetWorld(), *GetWorld()->GetMapName(), PlayersInitialized, ExpectedPlayers);
	UE_LOG(LogTemp, Warning, TEXT("[GM] HandleStartingNewPlayer_Implementation"));
	PlayersInitialized++;

	UE_LOG(LogTemp, Warning, TEXT("[GM] HandleStartingNewPlayer %d/%d"), PlayersInitialized, ExpectedPlayers);

	if (!bIsGameStarted && PlayersInitialized >= ExpectedPlayers)
	{
		// 모든 플레이어가 준비된 상황에서 실행
		UE_LOG(LogTemp, Warning, TEXT("[GM] HandleStartingNewPlayer_Implementation -> StartGame"));
		StartGame();
	}
}

void AER_InGameMode::DisConnectClient(APlayerController* PC)
{
	if (!PC) return;

	if (ABasePlayerController* ERPC = Cast<ABasePlayerController>(PC))
	{
		ERPC->Client_ReturnToMainMenu(TEXT("GameOver"));
	}

	FTimerHandle Tmp;
	GetWorld()->GetTimerManager().SetTimer(Tmp, [this, PC]()
		{
			if (GameSession)
			{
				GameSession->KickPlayer(PC, FText::FromString(TEXT("Defeated")));
			}
		}, 0.2f, false);
}

void AER_InGameMode::StartGame()
{
	if (bIsGameStarted) 
	{
		return;
	}
	bIsGameStarted = true;

	UE_LOG(LogTemp, Warning, TEXT("[GM] All players ready! Starting game init..."));

	TWeakObjectPtr<AER_InGameMode> WeakThis(this);
	GetWorld()->GetTimerManager().SetTimerForNextTick([WeakThis]()
		{
			if (!WeakThis.IsValid()) return;
			WeakThis->StartGame_Internal();
		});
}

void AER_InGameMode::StartGame_Internal()
{
	AER_GameState* ERGS = GetGameState<AER_GameState>();
	if (!ERGS)
	{
		return;
	}

	ERGS->BuildTeamCache();

	UER_NeutralSpawnSubsystem* NeutralSS = GetWorld()->GetSubsystem<UER_NeutralSpawnSubsystem>();
	if (NeutralSS)
	{
		NeutralSS->InitializeSpawnPoints(NeutralClass);
		NeutralSS->FirstSpawnNeutral();
	}
	UER_ObjectSubsystem* ObjectSS = GetWorld()->GetSubsystem<UER_ObjectSubsystem>();
	if (ObjectSS)
	{
		ObjectSS->InitializeObjectPoints(ObjectClass);
		
	}

	HandlePhaseTimeUp();
}

void AER_InGameMode::EndGame()
{
	TWeakObjectPtr<AER_InGameMode> WeakThis(this);
	GetWorld()->GetTimerManager().SetTimerForNextTick([WeakThis]()
		{
			if (!WeakThis.IsValid()) return;
			WeakThis->EndGame_Internal();
		});
}

void AER_InGameMode::EndGame_Internal()
{
	if (AER_GameState* ERGS = GetGameState<AER_GameState>())
	{
		ERGS->RemoveTeamCache();
	}

	PlayersInitialized = 0;

	UE_LOG(LogTemp, Warning, TEXT("[GM] Player is Zero -> ServerTravel to Lobby"));

	GetWorld()->ServerTravel(TEXT("/Game/Level/Level_Lobby"), true);
}

void AER_InGameMode::NotifyPlayerDied(ACharacter* VictimCharacter, APlayerState* KillerPS, const TArray<APlayerState*>& Assists)
{
	if (!HasAuthority() || !VictimCharacter)
		return;

	UE_LOG(LogTemp, Warning, TEXT("[GM] : Start NotifyPlayerDied"));

	AER_PlayerState* ERPS = VictimCharacter->GetPlayerState<AER_PlayerState>();
	AER_PlayerState* KillerERPS = Cast<AER_PlayerState>(KillerPS);
	AER_GameState* ERGS = GetGameState<AER_GameState>();

	if (!ERPS || !ERGS)
		return;

	if (UER_RespawnSubsystem* RespawnSS = GetWorld()->GetSubsystem<UER_RespawnSubsystem>() )
	{
		RespawnSS->HandlePlayerDeath(*ERPS, *ERGS, KillerERPS, Assists);

		// 탈락 방지 페이즈인지 확인
		const int32 Phase = ERGS->GetCurrentPhase();
		const bool bCanEliminationProtect = (Phase == 1 || Phase == 2);

		// 전멸 판정
		if (!bCanEliminationProtect)
		{
			if (RespawnSS->EvaluateTeamElimination(*ERPS, *ERGS))
			{
				UE_LOG(LogTemp, Warning, TEXT("[GM] : NotifyPlayerDied , EvaluateTeamElimination = true"));

				// 전멸 판정 true -> 해당 유저의 팀 사출 실행
				const int32 TeamIdx = static_cast<int32>(ERPS->TeamType);

				// 해당 팀의 리스폰 타이머 정지
				RespawnSS->StopResapwnTimer(*ERGS, TeamIdx);

				// 해당 팀 패배 처리
				RespawnSS->SetTeamLose(*ERGS, TeamIdx);

				// 승리 팀 체크
				int32 LastTeamIdx = RespawnSS->CheckIsLastTeam(*ERGS);
				if (LastTeamIdx != -1)
				{
					RespawnSS->SetTeamWin(*ERGS, LastTeamIdx);
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[GM] : NotifyPlayerDied , EvaluateTeamElimination = false"));

				// 전멸 판정 false -> 리스폰 함수 실행
				RespawnSS->StartRespawnTimer(*ERPS, *ERGS);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[GM] : NotifyPlayerDied , EvaluateTeamElimination = false"));

			// 전멸 판정 false -> 리스폰 함수 실행
			RespawnSS->StartRespawnTimer(*ERPS, *ERGS);
		}
	}
}

void AER_InGameMode::NotifyNeutralDied(ACharacter* VictimCharacter)
{
	if (!HasAuthority() || !VictimCharacter)
		return;

	//임시니까 일단 캐릭터의 변수를 이용
	UE_LOG(LogTemp, Log, TEXT("[GM] : NotifyNeutralDied Start"));

	ABaseMonster* NC = Cast<ABaseMonster>(VictimCharacter);
	NC->GetSpawnPoint();
	int32 SpawnPoint = NC->GetSpawnPoint();
	UER_NeutralSpawnSubsystem* NeutralSS = GetWorld()->GetSubsystem<UER_NeutralSpawnSubsystem>();
	NeutralSS->SetFalsebIsSpawned(SpawnPoint);
	NeutralSS->StartRespawnNeutral(SpawnPoint);
}

void AER_InGameMode::HandlePhaseTimeUp()
{
	AER_GameState* ERGS = GetGameState<AER_GameState>();
	if (!ERGS)
	{
		return;
	}
	UER_PhaseSubsystem* PhaseSS = GetWorld()->GetSubsystem<UER_PhaseSubsystem>();
	if (!PhaseSS)
	{
		return;
	}
	if (ERGS->GetCurrentPhase() < 5)
	{
		ERGS->SetCurrentPhase(ERGS->GetCurrentPhase() + 1);
		// 페이즈에 따라 작동할 코드 넣기
		UER_ObjectSubsystem* ObjectSS = GetWorld()->GetSubsystem<UER_ObjectSubsystem>();
		if (ObjectSS)
		{
			// (항공 보급 생성)
			ObjectSS->SpawnSupplyObject();
			// (오브젝트 스폰)
			ObjectSS->SpawnBossObject();
		}
		
	}

	// 이후에 10초에서 180초로 수정
	PhaseSS->StartPhaseTimer(*ERGS, PhaseDuration);
	PhaseSS->StartNoticeTimer(PhaseDuration);
}

void AER_InGameMode::HandleObjectNoticeTimeUp()
{
	UER_ObjectSubsystem* ObjectSS = GetWorld()->GetSubsystem<UER_ObjectSubsystem>();
	if (ObjectSS)
	{
		// 항공 보급 생성 위치 알림
		ObjectSS->PickSupplySpawnIndex();
		ObjectSS->PickBossSpawnIndex();
	}
}

void AER_InGameMode::TEMP_SpawnNeutrals()
{
	UER_NeutralSpawnSubsystem* NeutralSS = GetWorld()->GetSubsystem<UER_NeutralSpawnSubsystem>();
	NeutralSS->TEMP_SpawnNeutrals();
}

void AER_InGameMode::TEMP_DespawnNeutrals()
{
	UER_NeutralSpawnSubsystem* NeutralSS = GetWorld()->GetSubsystem<UER_NeutralSpawnSubsystem>();
	NeutralSS->TEMP_NeutralsALLDespawn();
}
