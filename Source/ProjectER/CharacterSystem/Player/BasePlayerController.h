#pragma once

#include "ItemSystem/Interface/I_ItemInteractable.h" // [김현수 추가분]

//Curved World Subsystem added //2026/02/10
#include "CurvedWorldSubsystem.h"
#include "FCurvedWorldUtil.h"

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
	
	// UI 로직 내 호출을 위해 protected -> public 변경
	// 키 입력 시, 어빌리티 호출
	void AbilityInputTagPressed(FGameplayTag InputTag);
	void AbilityInputTagReleased(FGameplayTag InputTag);
	
protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;
	
	virtual void OnRep_Pawn() override;
	
	// 우클릭 이동 / 공격
	void OnMoveStarted();
	void OnMoveTriggered();
	void OnMoveReleased();
	void MoveToMouseCursor();
	void OnStopTriggered();

	//좌클릭 결정/취소
	void OnConfirm();
	void OnCanceled();
	
	// [테스트용] 팀 변경 함수
	// void Test_ChangeTeamToA();
	// void Test_ChangeTeamToB();
	
	// [테스트용] 부활 함수
	// void Test_ReviveInput();

	// [테스트] 경험치 획득
	void Test_GainXP();
	
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

public:
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

	UFUNCTION(BlueprintCallable, Client, Reliable)
	void Client_OutGameInputMode();

	UFUNCTION(BlueprintCallable, Client, Reliable)
	void Client_InGameInputMode();

	UFUNCTION(BlueprintCallable, Client, Reliable)
	void Client_ReturnToMainMenu(const FString& Reason);



	// 아이템 루팅 RPC

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_RequestPickup(class ABaseItemActor* Item);

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_BeginLoot(ABaseBoxActor* Box);// 루팅 시작 요청

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_EndLoot(ABaseBoxActor* Box); // 루팅 종료 요청(또는 거리 벗어남)

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_TakeItem(ABaseBoxActor* Box, int32 SlotIndex); // 아이템 가져가기(핵심)

	UFUNCTION(BlueprintCallable, Client, Reliable)
	void Client_OpenLootUI(const ABaseBoxActor* Box); // UI 열기(로컬 전용)

	UFUNCTION(BlueprintCallable, Client, Reliable)
	void Client_CloseLootUI(); // UI 닫기(로컬 전용)
	// 박스 아이템 루팅 RPC 끝


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

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_MoveTeam(int32 TeamIdx);

	// Helper function to get hit result with curved world correction
	bool GetCurvedHitResultUnderCursor(
		ECollisionChannel TraceChannel,
		bool bTraceComplex,
		FHitResult& OutHitResult);

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

	UPROPERTY(EditDefaultsOnly, Category = "ProjectER|UI")
	TSubclassOf<class UW_LootingPopup> LootWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<class UW_LootingPopup> LootWidgetInstance;


	// Add reference to curved world subsystem
	UPROPERTY()
	TObjectPtr<UCurvedWorldSubsystem> CurvedWorldSubsystem;//cahced subsystem
    

};
