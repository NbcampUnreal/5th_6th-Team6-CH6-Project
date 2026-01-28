
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ER_PlayerState.generated.h"

UENUM(BlueprintType)
enum class ETeam : uint8
{
	None UMETA(DisplayName = "None"),
	Team1 UMETA(DisplayName = "Team1"),
	Team2 UMETA(DisplayName = "Team2"),
	Team3 UMETA(DisplayName = "Team3"),
	Neutral UMETA(DisplayName = "Neutral")

};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnLoseChanged, bool);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnWinChanged, bool);

UCLASS()
class PROJECTER_API AER_PlayerState : public APlayerState
{
	GENERATED_BODY()
	AER_PlayerState();

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void CopyProperties(class APlayerState* PlayerState) override;

	UFUNCTION()
	void OnRep_IsLose();

	UFUNCTION()
	void OnRep_IsWin();


	// Getter
	UFUNCTION(BlueprintCallable)
	FString GetPlayerStateName() { return PlayerStateName; }

	UFUNCTION(BlueprintCallable)
	bool GetIsReady() { return bIsReady; }

	// Setter
	UFUNCTION(BlueprintCallable)
	void SetPlayerStateName(FString InputName) { PlayerStateName = InputName; }

	UFUNCTION(BlueprintCallable)
	void SetbIsReady() { bIsReady = !bIsReady; }





public:
	UPROPERTY(Replicated, BlueprintReadOnly)
	FString PlayerStateName;

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsReady = false;

	UPROPERTY(Replicated, BlueprintReadOnly)
	ETeam Team = ETeam::None;

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsDead = false;


	UPROPERTY(ReplicatedUsing = OnRep_IsLose, BlueprintReadOnly)
	bool bIsLose = false;

	UPROPERTY(ReplicatedUsing = OnRep_IsWin, BlueprintReadOnly)
	bool bIsWin = false;


	FOnLoseChanged OnLoseChanged;
	FOnWinChanged OnWinChanged;

	
};
