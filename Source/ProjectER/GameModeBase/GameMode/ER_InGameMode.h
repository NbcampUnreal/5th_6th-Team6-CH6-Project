#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CharacterSystem/Interface/TargetableInterface.h"
#include "ER_InGameMode.generated.h"


class UER_NeutralSpawnSubsystem;

USTRUCT(BlueprintType)
struct FNeutralClassConfig
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<ACharacter> Class;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RespawnDelay = 5.f;
};

USTRUCT(BlueprintType)
struct FObjectClassConfig
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> Class;
};

/** 끊긴 플레이어의 보존 데이터 */
USTRUCT()
struct FDisconnectedPlayerData
{
	GENERATED_BODY()

	/** 끊기기 전 소유하던 Pawn */
	UPROPERTY()
	TWeakObjectPtr<APawn> PreservedPawn;

	/** 타임아웃 타이머 핸들 */
	FTimerHandle TimeoutHandle;

	// ── 보존할 Attribute 데이터 ──
	UPROPERTY()
	TMap<FString, float> SavedAttributes;

	// ── 보존할 PlayerState 데이터 ──
	UPROPERTY()
	TArray<class UBaseItemData*> SavedInventory;

	FString PlayerStateName;
	ETeamType TeamType = ETeamType::None;
	bool bIsDead = false;
	bool bIsLose = false;
	bool bIsWin = false;
	float RespawnTime = 5.f;
	int32 KillCount = 0;
	int32 DeathCount = 0;
	int32 AssistCount = 0;
};

UCLASS()
class PROJECTER_API AER_InGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	AER_InGameMode();
	
	virtual void BeginPlay() override;
	virtual void PostSeamlessTravel() override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void Logout(AController* Exiting) override;

	/** 신규 플레이어 입장 제어 (재접속 허용 / 신규 차단) */
	virtual void PreLogin(const FString& InAddress, const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	/** 재접속 플레이어에게 기존 Pawn 재연결 */
	virtual void PostLogin(APlayerController* NewPlayer) override;

	void HandlePlayerLoadComplete(APlayerController* PC);

	void StartGame();
	void StartGame_Initialize();
	void StartGame_Internal();
	void EndGame();
	void EndGame_Internal();

	UFUNCTION(BlueprintCallable)
	void NotifyPlayerDied(ACharacter* VictimCharacter, APlayerState* KillerPS, const TArray<APlayerState*>& Assists);

	UFUNCTION(BlueprintCallable)
	void NotifyNeutralDied(ACharacter* VictimCharacter);

	UFUNCTION(BlueprintCallable)
	void DisConnectClient(APlayerController* PC);

	void HandlePhaseTimeUp();

	void HandleObjectNoticeTimeUp();


	UFUNCTION(BlueprintCallable)
	void TEMP_SpawnNeutrals();

	UFUNCTION(BlueprintCallable)
	void TEMP_DespawnNeutrals();


private:
	bool bIsGameStarted = false;
	int32 PlayersInitialized = 0; // Number of players connected
	int32 PlayersReady = 0;       // Number of players who finished preloading
	int32 ExpectedPlayers = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Phase")
	float PhaseDuration = 30.f;

	UPROPERTY(EditDefaultsOnly, Category = "Subsystem|Neutral")
	TMap<FName, FNeutralClassConfig> NeutralClass;

	FTimerHandle LoadingTimeoutHandle;
	void HandleLoadingTimeout();

	FTimerHandle StartCountdownTimerHandle;
	int32 RemainingSeconds = 5;
	void TickCountdown();

	UPROPERTY(EditDefaultsOnly, Category = "Subsystem|Object")
	TMap<FName, FObjectClassConfig> ObjectClass;

	// ── Reconnect System ──

	/** 끊긴 플레이어 데이터 보존 맵 (Key: UniqueNetId 문자열) */
	TMap<FString, FDisconnectedPlayerData> DisconnectedPlayers;

	/** 타임아웃 시 끊긴 플레이어의 보존 데이터 정리 */
	void CleanupDisconnectedPlayer(const FString& UniqueIdStr);

	/** 재접속 대기 제한 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "Reconnect")
	float ReconnectTimeoutSeconds = 60.0f;
};

