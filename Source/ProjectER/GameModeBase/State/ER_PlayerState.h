
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "CharacterSystem/Interface/TargetableInterface.h"
#include "CharacterSystem/Data/CharacterData.h"
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

// [추가] 캐릭터 데이터가 변경되었음을 UI에 알리는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterDataChanged, TSoftObjectPtr<UCharacterData>, NewCharacterData);

// [추가] 준비 상태가 변경되었음을 UI에 알리는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnReadyStateChanged, bool, bNewReadyState);

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

	UFUNCTION(BlueprintCallable)
	int32 GetStartPoint() const { return StartPoint; }

	UFUNCTION(BlueprintCallable)
	void SetStartPoint(int32 idx) { StartPoint = idx; }

	UFUNCTION(BlueprintCallable, Category = "Character Selection")
	TSoftObjectPtr<UCharacterData> GetSelectedCharacterData() const { return SelectedCharacterData; }

	UFUNCTION(BlueprintCallable, Category = "Character Selection")
	void SetSelectedCharacterData(TSoftObjectPtr<UCharacterData> InData);

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_SetStartPoint(int32 idx);

	UFUNCTION(BlueprintCallable)// added for the state comp setting 2026/03/05
	ETeamType GetTeamType() const {return TeamType;}

	// Setter
	UFUNCTION(BlueprintCallable)
	void SetPlayerStateName(FString InputName) { PlayerStateName = InputName; }

	// [수정] 블루프린트에서 호출하는 대신 Server RPC를 통해 서버 쪽에서 SetReadyState를 부르도록 변경
	UFUNCTION(BlueprintCallable)
	void SetReadyState(bool bNewReadyState);

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

	// [수정] 준비 상태 변경 시 OnRep 실행되도록 변경
	UPROPERTY(ReplicatedUsing = OnRep_bIsReady, BlueprintReadOnly, Category = "Lobby")
	bool bIsReady = false;

	UFUNCTION()
	void OnRep_bIsReady();

	UPROPERTY(BlueprintAssignable, Category = "Lobby")
	FOnReadyStateChanged OnReadyStateChanged;

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

	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 StartPoint = 0;
	
	UPROPERTY(Replicated, BlueprintReadOnly)
	float CurrentRestrictedTime = 10.f;

	UFUNCTION(BlueprintCallable, Client, Reliable)
	void setUI_RestrictedTime();

	// [수정] 값이 복제될 때 클라에서 OnRep 함수가 자동 실행되도록 ReplicatedUsing으로 변경
	UPROPERTY(ReplicatedUsing = OnRep_SelectedCharacterData, BlueprintReadOnly, Category = "Character Selection")
	TSoftObjectPtr<UCharacterData> SelectedCharacterData;

	UFUNCTION()
	void OnRep_SelectedCharacterData();

	// [추가] UI 위젯 구동용 이벤트 디스패처
	UPROPERTY(BlueprintAssignable, Category = "Character Selection")
	FOnCharacterDataChanged OnCharacterDataChanged;

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
