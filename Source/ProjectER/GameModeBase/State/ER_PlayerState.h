
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
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

class UAbilitySystemComponent;
class UBaseAttributeSet;

UCLASS()
class PROJECTER_API AER_PlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
	AER_PlayerState();

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void CopyProperties(class APlayerState* PlayerState) override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UBaseAttributeSet* GetAttributeSet() const { return AttributeSet; }

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

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsLose = false;

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsWin = false;

	UPROPERTY(Replicated, BlueprintReadOnly)
	float RespawnTime = 5.f;

private:
	UPROPERTY(VisibleAnywhere, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, Category = "GAS")
	TObjectPtr<UBaseAttributeSet> AttributeSet;

};
