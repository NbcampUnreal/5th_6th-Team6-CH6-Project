
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "CharacterSystem/Interface/TargetableInterface.h"
#include "ER_PlayerState.generated.h"


class UAbilitySystemComponent;
class UBaseAttributeSet;


USTRUCT()
struct FDamageContrib
{
	GENERATED_BODY()

	TWeakObjectPtr<APlayerState> AttackerPS;
	float LastHitTime = 0.f;
	float TotalDamage = 0.f;
};

UCLASS()
class PROJECTER_API AER_PlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
	AER_PlayerState();

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void CopyProperties(class APlayerState* PlayerState) override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	void AddDamageContributor(APlayerState* AttackerPS, float Damage, float Now);
	void GetAssists(float Now, float WindowSec, APlayerState* KillerPS, TArray<APlayerState*>& OutAssists) const;
	void ResetDamageContrib();

	// Getter
	UBaseAttributeSet* GetAttributeSet() const { return AttributeSet; }

	UFUNCTION(BlueprintCallable)
	FString GetPlayerStateName() { return PlayerStateName; }

	UFUNCTION(BlueprintCallable)
	bool GetIsReady() { return bIsReady; }

	UFUNCTION(BlueprintCallable)
	int32 GetKillCount() { return KillCount; }

	UFUNCTION(BlueprintCallable)
	int32 GetDeathCount() { return DeathCount; }

	UFUNCTION(BlueprintCallable)
	int32 GetAssistCount() { return AssistCount; }


	// Setter
	UFUNCTION(BlueprintCallable)
	void SetPlayerStateName(FString InputName) { PlayerStateName = InputName; }

	UFUNCTION(BlueprintCallable)
	void SetbIsReady() { bIsReady = !bIsReady; }

	UFUNCTION(BlueprintCallable)
	void AddKillCount() { ++KillCount; UE_LOG(LogTemp, Log, TEXT("[PS] AddKillCount"));
	}
	UFUNCTION(BlueprintCallable)
	void AddDeathCount() { ++DeathCount; UE_LOG(LogTemp, Log, TEXT("[PS] AddDeathCount"));
	}
	UFUNCTION(BlueprintCallable)
	void AddAssistCount() { ++AssistCount; }


public:
	UPROPERTY(Replicated, BlueprintReadOnly)
	FString PlayerStateName;

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsReady = false;

	UPROPERTY(Replicated, BlueprintReadOnly)
	ETeamType TeamType = ETeamType::None;

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsDead = false;

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsLose = false;

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsWin = false;

	UPROPERTY(Replicated, BlueprintReadOnly)
	float RespawnTime = 5.f;

	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 KillCount = 0;

	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 DeathCount = 0;

	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 AssistCount = 0;

private:
	UPROPERTY(VisibleAnywhere, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, Category = "GAS")
	TObjectPtr<UBaseAttributeSet> AttributeSet;

	UPROPERTY()
	TMap<TWeakObjectPtr<APlayerState>, FDamageContrib> DamageContribMap;

	UPROPERTY()
	TArray<APlayerState*> OutAssistArray;
};
