#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
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

UCLASS()
class PROJECTER_API AER_InGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	virtual void BeginPlay() override;
	virtual void PostSeamlessTravel() override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void Logout(AController* Exiting) override;

	void StartGame();
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
	int32 PlayersInitialized = 0;
	int32 ExpectedPlayers = 0;
	float PhaseDuration = 30.f;

	UPROPERTY(EditDefaultsOnly, Category = "Subsystem|Neutral")
	TMap<FName, FNeutralClassConfig> NeutralClass;

	UPROPERTY(EditDefaultsOnly, Category = "Subsystem|Object")
	TMap<FName, FObjectClassConfig> ObjectClass;

};
