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
#include "GameModeBase/Subsystem/Session/ER_SessionSubsystem.h" // 호스트 스팀 세션 파괴 위해 추가
#include "Engine/GameInstance.h"

#include "Monster/BaseMonster.h"

#include "CharacterSystem/Player/BasePlayerController.h"
#include "CharacterSystem/Character/BaseCharacter.h"
#include "AbilitySystemComponent.h"
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"
#include "ItemSystem/Component/BaseInventoryComponent.h"
#include "ItemSystem/Data/BaseItemData.h"
#include "CharacterSystem/Data/CharacterData.h"

#include "LevelManagement/LevelGraphManager/LevelAreaGameStateComp/LevelAreaGameStateComponent.h"
#include "LevelManagement/LevelAreaTrackerComponent.h"

void AER_InGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (UKismetSystemLibrary::IsDedicatedServer(GetWorld()))
	{
		return;
	}


}

AER_InGameMode::AER_InGameMode()
{
	bUseSeamlessTravel = true;
}

void AER_InGameMode::PostSeamlessTravel()
{
	Super::PostSeamlessTravel();

	UE_LOG(LogTemp, Warning, TEXT("[GM] PostSeamlessTravel - Expecting %d players"), ExpectedPlayers);

	// 무한 로딩을 방지하기 위해 60초 타이머 설정
	GetWorld()->GetTimerManager().SetTimer(LoadingTimeoutHandle, this, &AER_InGameMode::HandleLoadingTimeout, 60.0f, false);
	
	// Notify Blueprint that we have entered a new level
	OnLevelInstanceLoaded();
}

void AER_InGameMode::HandleLoadingTimeout()
{
	if (bIsGameStarted) return;

	UE_LOG(LogTemp, Error, TEXT("[GM] Loading timeout! Arrrived: %d / Ready: %d / Expected: %d"), PlayersInitialized, PlayersReady, ExpectedPlayers);

	if (PlayersReady > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GM] Forcing start with %d ready players."), PlayersReady);
		ExpectedPlayers = PlayersReady;
		StartGame();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[GM] Not enough players. Aborting match."));
		EndGame();
	}
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
	// ── 게임 중 끊긴 플레이어 데이터 보존 (Super 호출 전에 처리) ──
	if (bIsGameStarted && !bIsGameEnd)
	{
		APlayerController* PC = Cast<APlayerController>(Exiting);
		if (PC && PC->PlayerState)
		{
			const FUniqueNetIdRepl UniqueId = PC->PlayerState->GetUniqueId();
			const FString UniqueIdStr = UniqueId.IsValid() ? UniqueId->ToString() : FString();

			if (!UniqueIdStr.IsEmpty())
			{
				FDisconnectedPlayerData Data;

				// Pawn 보존: 컨트롤러에서 분리만 하고 파괴하지 않음
				APawn* OwnedPawn = PC->GetPawn();
				if (!OwnedPawn)
				{
					if (ABasePlayerController* ERPC = Cast<ABasePlayerController>(PC))
					{
						OwnedPawn = ERPC->GetControlledBaseChar();
					}
				}

				if (OwnedPawn)
				{
					// 이미 UnPossess 되었을 수 있으나 안 되어 있다면 수행
					if (OwnedPawn->GetController() == PC)
					{
						PC->UnPossess();
					}

					// ★ 핵심: Owner 체인을 끊어야 PC 파괴 시 Pawn이 같이 파괴되지 않음
					// UnPossess()는 Controller 포인터만 null로 만들 뿐, Owner는 여전히 PC를 가리킴
					OwnedPawn->SetOwner(nullptr);

					Data.PreservedPawn = OwnedPawn;
				}

				// PlayerState 데이터 캡처
				AER_PlayerState* ERPS = Cast<AER_PlayerState>(PC->PlayerState);
				if (ERPS)
				{
					Data.PlayerStateName = ERPS->PlayerStateName;
					Data.TeamType        = ERPS->TeamType;
					Data.bIsDead         = ERPS->bIsDead;
					Data.bIsLose         = ERPS->bIsLose;
					Data.bIsWin          = ERPS->bIsWin;
					Data.RespawnTime     = ERPS->RespawnTime;
					Data.KillCount       = ERPS->KillCount;
					Data.DeathCount      = ERPS->DeathCount;
					Data.AssistCount     = ERPS->AssistCount;

					// ASC Attribute 데이터 추출
					if (UAbilitySystemComponent* ASC = ERPS->GetAbilitySystemComponent())
					{
						for (TFieldIterator<FProperty> It(UBaseAttributeSet::StaticClass()); It; ++It)
						{
							if (FStructProperty* StructProp = CastField<FStructProperty>(*It))
							{
								if (StructProp->Struct == FGameplayAttributeData::StaticStruct())
								{
									FGameplayAttribute Attribute(*It);
									Data.SavedAttributes.Add(It->GetName(), Attribute.GetNumericValue(ERPS->GetAttributeSet()));
								}
							}
						}
						
						UE_LOG(LogTemp, Warning, TEXT("[GM] Logout >> Captured %d attributes for player: %s"), 
							Data.SavedAttributes.Num(), *UniqueIdStr);
					}

					// 인벤토리 데이터 추출
					if (UBaseInventoryComponent* Inv = OwnedPawn->FindComponentByClass<UBaseInventoryComponent>())
					{
						// Internal array 직접 접근을 못하므로 getter가 필요할 수 있으나, 
						// .h에서 InventoryContents가 protected이므로 헬퍼가 필요함.
						// 일단 리플렉션이나 다른 방법을 고려하거나, .h를 수정하여 접근 허용.
						// (이미 h에서 protected이므로 파생클래스가 아니면 접근 불가)
						// 하지만 TFieldIterator로 가져올 수 있음.
						for (TFieldIterator<FArrayProperty> It(UBaseInventoryComponent::StaticClass()); It; ++It)
						{
							if (It->GetName() == TEXT("InventoryContents"))
							{
								FScriptArrayHelper Helper(*It, It->ContainerPtrToValuePtr<void>(Inv));
								for (int32 i = 0; i < Helper.Num(); ++i)
								{
									UBaseItemData* Item = *reinterpret_cast<UBaseItemData**>(Helper.GetRawPtr(i));
									Data.SavedInventory.Add(Item);
								}
								break;
							}
						}
						
						UE_LOG(LogTemp, Warning, TEXT("[GM] Logout >> Captured %d inventory items for player: %s"), 
							Data.SavedInventory.Num(), *UniqueIdStr);
					}
				}

				// 타임아웃 타이머 설정
				GetWorld()->GetTimerManager().SetTimer(
					Data.TimeoutHandle, 
					[this, UniqueIdStr]() { CleanupDisconnectedPlayer(UniqueIdStr); },
					ReconnectTimeoutSeconds, false);

				DisconnectedPlayers.Add(UniqueIdStr, Data);

				UE_LOG(LogTemp, Warning, TEXT("[GM] Player disconnected mid-game. Preserving data for reconnect: %s (Timeout: %.0fs)"), 
					*UniqueIdStr, ReconnectTimeoutSeconds);
			}
		}
	}

	Super::Logout(Exiting);

	// 남아있는 플레이어 컨트롤러 수 계산
	int32 RemainingPlayers = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		// 방금 Logout을 호출한 Exiting은 제외
		if (IsValid(PC) && PC != Exiting && PC->PlayerState)
		{
			++RemainingPlayers;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[GM] Logout. RemainingPlayers=%d"), RemainingPlayers);

	// 게임이 종료된 상태면 승패/인원 관련 처리를 더 이상 진행하지 않음
	if (bIsGameEnd)
	{
		return;
	}

	if (!bIsGameStarted)
	{
		// 로딩/대기 중 나감
		ExpectedPlayers = FMath::Max(0, ExpectedPlayers - 1);
		UE_LOG(LogTemp, Warning, TEXT("[GM] Logout before start. Adjusted Expected= %d"), ExpectedPlayers);

		if (ExpectedPlayers <= 1 || RemainingPlayers <= 1)
		{
			UE_LOG(LogTemp, Warning, TEXT("[GM] Not enough players to start. Ending game."));
			GetWorld()->GetTimerManager().ClearTimer(LoadingTimeoutHandle);
			EndGame();
		}
		else if (PlayersReady >= ExpectedPlayers)
		{
			UE_LOG(LogTemp, Warning, TEXT("[GM] All remaining players loaded. Starting game."));
			GetWorld()->GetTimerManager().ClearTimer(LoadingTimeoutHandle);
			StartGame();
		}
	}
	else
	{
		// 게임 중 나감 — 재접속 대기 중인 플레이어를 제외하고 남은 인원 계산
		const int32 TotalActive = RemainingPlayers + DisconnectedPlayers.Num();
		if (TotalActive < 1)
		{
			EndGame();
		}
	}
}

void AER_InGameMode::PreLogin(const FString& InAddress, const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	// 게임 시작 전(로비)이면 무조건 통과
	if (!bIsGameStarted)
	{
		Super::PreLogin(InAddress, Options, UniqueId, ErrorMessage);
		return;
	}

	// 게임 진행 중인 경우: 재접속 플레이어만 허용
	const FString UniqueIdStr = UniqueId.IsValid() ? UniqueId->ToString() : FString();

	if (!UniqueIdStr.IsEmpty() && DisconnectedPlayers.Contains(UniqueIdStr))
	{
		UE_LOG(LogTemp, Warning, TEXT("[GM] PreLogin >> Reconnecting player allowed: %s"), *UniqueIdStr);
		Super::PreLogin(InAddress, Options, UniqueId, ErrorMessage);
		return;
	}

	// 신규 플레이어 차단
	UE_LOG(LogTemp, Warning, TEXT("[GM] PreLogin >> New player rejected (game in progress): %s"), *UniqueIdStr);
	ErrorMessage = TEXT("Game is already in progress. New players cannot join.");
}

void AER_InGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!NewPlayer || !NewPlayer->PlayerState)
		return;

	const FUniqueNetIdRepl UniqueId = NewPlayer->PlayerState->GetUniqueId();
	const FString UniqueIdStr = UniqueId.IsValid() ? UniqueId->ToString() : FString();

	// 재접속 플레이어인지 확인
	FDisconnectedPlayerData* FoundData = DisconnectedPlayers.Find(UniqueIdStr);
	if (!FoundData)
		return;

	UE_LOG(LogTemp, Warning, TEXT("[GM] PostLogin >> Reconnecting player detected: %s"), *UniqueIdStr);

	// 타임아웃 타이머 해제
	GetWorld()->GetTimerManager().ClearTimer(FoundData->TimeoutHandle);

	// 보존된 Pawn 재연결
	if (FoundData->PreservedPawn.IsValid())
	{
		// Super::PostLogin() 에서 생성된 새로운 Pawn이 있다면 제거
		if (APawn* NewPawn = NewPlayer->GetPawn())
		{
			if (NewPawn != FoundData->PreservedPawn.Get())
			{
				NewPawn->Destroy();
			}
		}

		NewPlayer->Possess(FoundData->PreservedPawn.Get());
		UE_LOG(LogTemp, Warning, TEXT("[GM] PostLogin >> Pawn restored for: %s"), *UniqueIdStr);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[GM] PostLogin >> No preserved Pawn found for: %s (destroyed or timed out)"), *UniqueIdStr);
	}

	// PlayerState 데이터 복원
	AER_PlayerState* NewERPS = Cast<AER_PlayerState>(NewPlayer->PlayerState);
	if (NewERPS)
	{
		NewERPS->PlayerStateName = FoundData->PlayerStateName;
		NewERPS->TeamType        = FoundData->TeamType;
		NewERPS->bIsDead         = FoundData->bIsDead;
		NewERPS->bIsLose         = FoundData->bIsLose;
		NewERPS->bIsWin          = FoundData->bIsWin;
		NewERPS->RespawnTime     = FoundData->RespawnTime;
		NewERPS->KillCount       = FoundData->KillCount;
		NewERPS->DeathCount      = FoundData->DeathCount;
		NewERPS->AssistCount     = FoundData->AssistCount;

		// Attribute 데이터 복원
		if (UAbilitySystemComponent* ASC = NewERPS->GetAbilitySystemComponent())
		{
			// ASC ActorInfo가 먼저 설정되어 있어야 함 (Possess 이후 시점이므로 안전)
			for (const auto& Pair : FoundData->SavedAttributes)
			{
				FGameplayAttribute Attribute;
				for (TFieldIterator<FProperty> It(UBaseAttributeSet::StaticClass()); It; ++It)
				{
					if (It->GetName() == Pair.Key)
					{
						Attribute = FGameplayAttribute(*It);
						break;
					}
				}

				if (Attribute.IsValid())
				{
					ASC->SetNumericAttributeBase(Attribute, Pair.Value);
					// 현재 값도 동일하게 맞춤 (GE에 인한 보정 전 기본값)
					ASC->SetNumericAttributeBase(Attribute, Pair.Value); 
				}
			}

			UE_LOG(LogTemp, Warning, TEXT("[GM] PostLogin >> Restored %d attributes for player: %s"), 
				FoundData->SavedAttributes.Num(), *UniqueIdStr);
		}

		// 인벤토리 데이터 복원
		if (APawn* PreservedPawn = FoundData->PreservedPawn.Get())
		{
			if (UBaseInventoryComponent* Inv = PreservedPawn->FindComponentByClass<UBaseInventoryComponent>())
			{
				for (TFieldIterator<FArrayProperty> It(UBaseInventoryComponent::StaticClass()); It; ++It)
				{
					if (It->GetName() == TEXT("InventoryContents"))
					{
						FScriptArrayHelper Helper(*It, It->ContainerPtrToValuePtr<void>(Inv));
						Helper.EmptyAndAddValues(FoundData->SavedInventory.Num());
						for (int32 i = 0; i < FoundData->SavedInventory.Num(); ++i)
						{
							*reinterpret_cast<UBaseItemData**>(Helper.GetRawPtr(i)) = FoundData->SavedInventory[i];
						}
						break;
					}
				}
				
				UE_LOG(LogTemp, Warning, TEXT("[GM] PostLogin >> Restored %d inventory items for player: %s"), 
					FoundData->SavedInventory.Num(), *UniqueIdStr);
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("[GM] PostLogin >> PlayerState restored for: %s (Team=%d, K/D/A=%d/%d/%d)"),
			*UniqueIdStr, static_cast<int32>(NewERPS->TeamType), NewERPS->KillCount, NewERPS->DeathCount, NewERPS->AssistCount);
	}

	// GameState TeamCache에 재접속 플레이어 다시 추가
	AER_GameState* ERGS = GetGameState<AER_GameState>();
	if (ERGS && NewERPS)
	{
		const int32 TeamIdx = static_cast<int32>(NewERPS->TeamType);
		TArray<FString>& TeamArr = ERGS->GetTeamArray(TeamIdx);
		
		const FUniqueNetIdRepl UID = NewERPS->GetUniqueId();
		FString NewPlayerUniqueIdStr = UID.IsValid() ? UID->ToString() : NewERPS->GetPlayerName();
		if (!NewPlayerUniqueIdStr.IsEmpty())
		{
			TeamArr.AddUnique(NewPlayerUniqueIdStr);
		}
	}

	// 재접속 플레이어에게 인게임 입력 모드 및 프리로드 지시
	if (ABasePlayerController* ERPC = Cast<ABasePlayerController>(NewPlayer))
	{
		ERPC->Client_InGameInputMode();
		ERPC->Client_StartPreload();
	}

	// 보존 데이터 제거
	DisconnectedPlayers.Remove(UniqueIdStr);
}

void AER_InGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	if (!NewPlayer)
		return;

	// 재접속 플레이어는 초기화 카운트에 포함하지 않음 (PostLogin에서 별도 처리)
	if (bIsGameStarted)
		return;

	if (ABasePlayerController* PC = Cast<ABasePlayerController>(NewPlayer))
	{
		PC->Client_InGameInputMode();
		PC->Client_StartPreload(); // 방에 들어온 클라이언트에게 에셋 로딩을 지시
	}
	UE_LOG(LogTemp, Warning, TEXT("[GM] HSNPlayer this=%p world=%p map=%s PI=%d/%d"),
		this, GetWorld(), *GetWorld()->GetMapName(), PlayersInitialized, ExpectedPlayers);

	PlayersInitialized++;

	UE_LOG(LogTemp, Warning, TEXT("[GM] HandleStartingNewPlayer (Connected: %d/%d)"), PlayersInitialized, ExpectedPlayers);

	// 게임 진입 판단은 로딩 완료 이후에 진행
}

void AER_InGameMode::HandlePlayerLoadComplete(APlayerController* PC)
{
	PlayersReady++;

	UE_LOG(LogTemp, Warning, TEXT("[GM] HandlePlayerLoadComplete -> %d / %d Ready"), PlayersReady, ExpectedPlayers);

	if (!bIsGameStarted && PlayersReady >= ExpectedPlayers)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GM] All players ready -> StartGame"));
		GetWorld()->GetTimerManager().ClearTimer(LoadingTimeoutHandle);
		StartGame();
	}
}

APawn* AER_InGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer);
	if (!PawnClass)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = GetInstigator();
	SpawnInfo.ObjectFlags |= RF_Transient;
	SpawnInfo.bDeferConstruction = true; // 지연 스폰을 통해 BeginPlay 이전에 데이터 세팅

	APawn* ResultPawn = GetWorld()->SpawnActor<APawn>(PawnClass, SpawnTransform, SpawnInfo);
	if (ResultPawn)
	{
		if (ABaseCharacter* BaseChar = Cast<ABaseCharacter>(ResultPawn))
		{
			if (AER_PlayerState* ERPS = Cast<AER_PlayerState>(NewPlayer->PlayerState))
			{
				if (!ERPS->GetSelectedCharacterData().IsNull())
				{
					// 소프트 참조를 동기식으로 로드하여 HeroData에 주입
					BaseChar->HeroData = ERPS->GetSelectedCharacterData().LoadSynchronous();
					UE_LOG(LogTemp, Log, TEXT("[GM] Injected HeroData into spawned character"));
				}
			}
		}

		// 건설 및 BeginPlay 실행
		UGameplayStatics::FinishSpawningActor(ResultPawn, SpawnTransform);
	}

	return ResultPawn;
}

void AER_InGameMode::DisConnectClient(APlayerController* PC)
{
	if (!PC) return;

	if (ABasePlayerController* ERPC = Cast<ABasePlayerController>(PC))
	{
		ERPC->Client_ReturnToMainMenu(TEXT("GameOver"));
	}

	// 호스트(Listen Server 본인)는 자신을 Kick 할 수 없습니다. 
	// 호스트는 OpenLevel을 통해 메인 메뉴로 이동하면 자동으로 방이 터지고 넷드라이버가 닫힙니다.
	if (PC->IsLocalController())
	{
		return;
	}

	TWeakObjectPtr<APlayerController> WeakPC(PC);
	TWeakObjectPtr<AER_InGameMode> WeakThis(this);

	FTimerHandle Tmp;
	GetWorld()->GetTimerManager().SetTimer(Tmp, [WeakThis, WeakPC]()
		{
			if (WeakThis.IsValid() && WeakThis->GameSession && WeakPC.IsValid())
			{
				WeakThis->GameSession->KickPlayer(WeakPC.Get(), FText::FromString(TEXT("Defeated")));
			}
		}, 0.2f, false);
}

void AER_InGameMode::ShutdownServerForHost()
{
	UE_LOG(LogTemp, Warning, TEXT("[GM] Host requested server shutdown. Evacuating clients..."));

	// 1. 호스트를 제외한 모든 클라이언트에게 귀환 명령 송신
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		
		if (IsValid(PC) && !PC->IsLocalController()) 
		{
			if (ABasePlayerController* ERPC = Cast<ABasePlayerController>(PC))
			{
				ERPC->Client_ReturnToMainMenu(TEXT("Host Closed Server"));
			}
		}
	}

	// 2. 1초 대기 후 호스트 본인도 로비로 이동하며 방 폭파
	FTimerHandle ShutdownTimer;
	GetWorld()->GetTimerManager().SetTimer(ShutdownTimer, [this]()
	{
		if (APlayerController* HostPC = GetWorld()->GetFirstPlayerController())
		{
			if (ABasePlayerController* HostERPC = Cast<ABasePlayerController>(HostPC))
			{
				HostERPC->Client_ReturnToMainMenu(TEXT("Server Shutdown Complete"));
			}
		}
	}, 1.0f, false);
}

void AER_InGameMode::RequestTeleportToRegion(ACharacter* TargetCharacter, int32 RegionIndex)
{
	if (!TargetCharacter || !HasAuthority())
	{
		return;
	}

	UER_RespawnSubsystem* RespawnSS = GetWorld()->GetSubsystem<UER_RespawnSubsystem>();
	if (RespawnSS)
	{
		FTransform DestTransform = RespawnSS->GetRespawnPointLocation(RegionIndex);
		
		TargetCharacter->SetActorTransform(DestTransform, false, nullptr, ETeleportType::TeleportPhysics);

		UE_LOG(LogTemp, Warning, TEXT("[GM] Teleported Character %s to Region %d"), *TargetCharacter->GetName(), RegionIndex);
	}
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
			WeakThis->StartGame_Initialize();
		});

	UER_RespawnSubsystem* RespawnSS = GetWorld()->GetSubsystem<UER_RespawnSubsystem>();
	if (RespawnSS)
	{
		RespawnSS->InitializeRespawnPoints();
		RespawnSS->ResetAssignedSpawnPoints();
	}

	// 게임 카운트다운 시작 시 로딩 화면 닫기
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ABasePlayerController* PC = Cast<ABasePlayerController>(It->Get()))
		{
			PC->Client_CloseLoadingUI();

			if (RespawnSS)
			{
				int32 Idx = 99;
				if (AER_PlayerState* PS = PC->GetPlayerState<AER_PlayerState>())
				{
					Idx = PS->GetStartPoint();
				}
				UE_LOG(LogTemp, Log, TEXT("[GM] PS->StartPoint = %d"), Idx);

				FTransform Location = RespawnSS->GetRespawnPointLocation(Idx);
				PC->GetPawn()->SetActorTransform(Location);

				if (ULevelAreaTrackerComponent* Tracker = PC->GetPawn()->FindComponentByClass<ULevelAreaTrackerComponent>())
				{
					Tracker->UpdateArea();
				}

				UE_LOG(LogTemp, Log, TEXT("[GM] Success Get SpawnPoint %d"), Idx);
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("[GM] Failed Get SpawnPoint"));
			}
		}
	}

	GetWorldTimerManager().SetTimer(StartCountdownTimerHandle, this, &AER_InGameMode::TickCountdown, 1.0f, true);
}

void AER_InGameMode::TickCountdown()
{
	UE_LOG(LogTemp, Warning, TEXT("Game Start Countdown: %d"), RemainingSeconds);

	RemainingSeconds--;

	if (RemainingSeconds < 0)
	{
		GetWorldTimerManager().ClearTimer(StartCountdownTimerHandle);
		StartGame_Internal();
	}
}

void AER_InGameMode::StartGame_Initialize()
{
	AER_GameState* ERGS = GetGameState<AER_GameState>();
	if (ERGS)
	{
		ERGS->BuildTeamCache();
	}

	UER_NeutralSpawnSubsystem* NeutralSS = GetWorld()->GetSubsystem<UER_NeutralSpawnSubsystem>();
	if (NeutralSS)
	{
		NeutralSS->InitializeSpawnPoints(NeutralClass);
	}

	UER_ObjectSubsystem* ObjectSS = GetWorld()->GetSubsystem<UER_ObjectSubsystem>();
	if (ObjectSS)
	{
		ObjectSS->InitializeObjectPoints(ObjectClass);
	}

	UER_RespawnSubsystem* RespawnSS = GetWorld()->GetSubsystem<UER_RespawnSubsystem>();
	if (RespawnSS)
	{
		RespawnSS->InitializeRespawnMap(*ERGS);
	}
}

void AER_InGameMode::StartGame_Internal()
{
	//플레이어 시작 위치 지정 코드를 여기에
	UER_NeutralSpawnSubsystem* NeutralSS = GetWorld()->GetSubsystem<UER_NeutralSpawnSubsystem>();
	if (NeutralSS)
	{
		NeutralSS->FirstSpawnNeutral();
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
	PlayersReady = 0;

	// 게임이 종료되어 로비로 돌아가기 전에 현재 호스트의 스팀 세션을 먼저 내립니다.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UER_SessionSubsystem* SessionSubsystem = GI->GetSubsystem<UER_SessionSubsystem>())
		{
			SessionSubsystem->DestroyGameSession();
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[GM] Player is Zero -> ServerTravel to Lobby"));

	GetWorld()->ServerTravel(TEXT("/Game/Level/Level_MainMenu"), true);
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
		if (!bCanEliminationProtect && RespawnSS->EvaluateTeamElimination(*ERPS, *ERGS))
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
				// 승리 판정 시 게임 종료를 알림
				bIsGameStarted = false;
				bIsGameEnd = true;
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
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[GM] : NotifyNeutralDied Start"));

	ABaseMonster* NC = Cast<ABaseMonster>(VictimCharacter);
	if (!NC)
	{
		return;
	}

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
		ULevelAreaGameStateComponent* AreaGSComp = ERGS->GetComponentByClass<ULevelAreaGameStateComponent>();
		if(ERGS->GetCurrentPhase() != 1)
		{
			// 1페이즈에는 금지 구역을 지정하지 않도록 수정
			AreaGSComp->SetPhase(ERGS->GetCurrentPhase());
		}

		//FString Text = "";
		//for (auto& aa : AreaGSComp->HazardOrder)
		//{
		//	Text.Append(" -> ");
		//	Text.AppendInt(aa);

		//}
		//UE_LOG(LogTemp, Log, TEXT("[GM] AreaGSComp->HazardOrder : %s"), *Text);

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

	// bp exposed function for comp reaction
	OnPhaseTimeUp(ERGS->GetCurrentPhase());
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

void AER_InGameMode::CleanupDisconnectedPlayer(const FString& UniqueIdStr)
{
	FDisconnectedPlayerData* FoundData = DisconnectedPlayers.Find(UniqueIdStr);
	if (!FoundData)
		return;

	UE_LOG(LogTemp, Warning, TEXT("[GM] Reconnect timeout expired for: %s. Cleaning up preserved data."), *UniqueIdStr);

	// 보존된 Pawn 파괴
	if (FoundData->PreservedPawn.IsValid())
	{
		FoundData->PreservedPawn->Destroy();
	}

	DisconnectedPlayers.Remove(UniqueIdStr);
}
