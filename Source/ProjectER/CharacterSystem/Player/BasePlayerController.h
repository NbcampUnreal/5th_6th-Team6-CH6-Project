#pragma once

#include "ItemSystem/I_ItemInteractable.h" // [김현수 추가분]

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h" 
#include "BasePlayerController.generated.h"

class UNiagaraSystem;
class UInputMappingContext;
class UInputConfig;
class UInputAction;
class ABaseCharacter;

struct FInputActionValue;

UCLASS()
class PROJECTER_API ABasePlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ABasePlayerController();
	
protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;
	
	// 우클릭 이동 / 공격
	void OnMoveStarted();
	void OnMoveTriggered();
	void OnMoveReleased();
	void MoveToMouseCursor();
	void OnStopTriggered();
	
	// 키 입력 시, 어빌리티 호출
	void AbilityInputTagPressed(FGameplayTag InputTag);
	void AbilityInputTagReleased(FGameplayTag InputTag);
	
protected:
	// 입력 매핑 컨텍스트 (IMC)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	//  입력 설정 데이터 에셋 (InputConfig)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputConfig> InputConfig;
	
	// 커서 클릭 이펙트 (FX Class)
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UNiagaraSystem> FXCursor;
	
private:
	// 마우스 입력 키다운 플래그
	uint32 bIsMousePressed : 1;
	
	// 이동할 위치 캐싱
	FVector CachedDestination;
	
	// 조종 중인 캐릭터 캐싱
	UPROPERTY(Transient)
	TObjectPtr<ABaseCharacter> ControlledBaseChar;
	
	// 네트워크 부하 방지를 위한 타이머 
	float LastRPCUpdateTime;

	// RPC 전송 간격 (0.1 = 초당 10번)
	const float RPCUpdateInterval = 0.1f;

/// [김현수 추가분] - 시작
protected:
	// 상호작용 타겟 캐싱
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectER|Item")
	TObjectPtr<AActor> InteractionTarget;

	// 거리 체크 로직 (PlayerTick에서 호출)
	void CheckInteractionDistance();

	// 기존 Move 함수를 확장하여 상호작용 판정 포함
	void ProcessMouseInteraction();
/// [김현수 추가분] - 끝

//-----------------------------------------------------------

/// [전민성 추가분] - 시작
public:
	UFUNCTION(BlueprintCallable, Category = "Network")
	void ConnectToDedicatedServer(const FString& Ip, int32 Port, const FString& PlayerName);

	// Clinet RPC
	UFUNCTION(BlueprintCallable, Client, Reliable)
	void Client_SetLose();

	UFUNCTION(BlueprintCallable, Client, Reliable)
	void Client_SetWin();

	UFUNCTION(BlueprintCallable, Client, Reliable)
	void Client_SetDead();

	UFUNCTION(BlueprintCallable, Client, Reliable)
	void Client_StartRespawnTimer();

	UFUNCTION(BlueprintCallable, Client, Reliable)
	void Client_StopRespawnTimer();

private:
	// UI 출력
	void ShowWinUI();

	void ShowLoseUI();

	void ShowRespawnTimerUI();

	void HideRespawnTimerUI();

	// Server RPC
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_StartGame();

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_DisConnectServer();

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_TEMP_SpawnNeutrals();

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_TEMP_DespawnNeutrals();


private:
	// UI 클래스
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> LoseUIClass;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> LoseUIInstance;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> WinUIClass;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> WinUIInstance;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> RespawnUIClass;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> RespawnUIInstance;
};
