#pragma once

#include "ItemSystem/Interface/I_ItemInteractable.h" // [김현수 추가분]

//Curved World Subsystem added //2026/02/10
#include "CurvedWorldSubsystem.h"
#include "FCurvedWorldUtil.h"

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h" 
#include "BasePlayerController.generated.h"

UENUM(BlueprintType)
enum class EAudioType : uint8
{
	Master,
	BGM,
	SFX,
	UI
};

class UNiagaraSystem;
class UInputMappingContext;
class UInputConfig;
class UInputAction;
class UDecalComponent;
class ABaseCharacter;
class UCharacterData; // [추가] 캐릭터 데이터 포워드 선언

class UTopDownCameraComp; //Camera Added

class UUI_MainHUD; // UI시스템 관리자

class ABaseItemActor; // [김현수 추가분]

struct FInputActionValue;


//Log
DECLARE_LOG_CATEGORY_EXTERN(Controller_Camera, Log, All);

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
	virtual void PawnLeavingGame() override;
	
	virtual void OnRep_Pawn() override;
	
	// 매 프레임 마우스 호버 상태를 체크하는 함수
	void CheckHoveredActor();
	
	// 우클릭 이동 / 공격
	void OnMoveStarted();
	void OnMoveTriggered();
	void OnMoveReleased();
	void MoveToMouseCursor();
	void OnStopTriggered();

	//좌클릭 결정/취소
	void OnConfirm();
	void OnCanceled();
	
	// A키 입력 처리
	void OnAttackModePressed();

	// 공격 모드 상태 해제 (우클릭 이동 시 취소용)
	void CancelAttackMode();

	// 공격 명령 (Attack Move) 로직 요청
	void RequestAttackMove(const FHitResult& Hit);


	//Camera Movement Added(handler)
	void OnCameraPanX(const FInputActionValue& Value);
	void OnCameraPanY(const FInputActionValue& Value);
	void OnCameraToggle();
	void OnCameraHold_Started();
	void OnCameraHold_Completed();

	// 미니맵 클릭 이동 _ mpyi
public:
	void OnMinimapClicked(FVector _TargetWorldPos);

	UFUNCTION(BlueprintCallable)
	class ABaseCharacter* GetControlledBaseChar() const { return ControlledBaseChar; }

	
protected:
	// 입력 매핑 컨텍스트 (IMC)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	//  입력 설정 데이터 에셋 (InputConfig)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputConfig> InputConfig;
	
	// [추가] 사거리 표시용 데칼 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UDecalComponent> AttackRangeDecal;
	
	// 커서 클릭 이펙트 (FX Class)
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UNiagaraSystem> FXCursor;

	// 현재 마우스가 올라가 있는 액터 캐싱 (최적화 및 이전 하이라이트 끄기 용도)
	UPROPERTY()
	TObjectPtr<AActor> CurrentHoveredActor;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Input")
	TEnumAsByte<ECollisionChannel> MouseTraceChannel=ECollisionChannel::ECC_Visibility;//Default as visibility
	
	//Main Camera Added here
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UTopDownCameraComp> TopDownCameraComp;
	

private:
	// 마우스 입력 키다운 플래그
	uint8 bIsMousePressed : 1;
	
	// 공격 모드(A키 누른 상태) 확인 플래그
	uint8 bIsAttackInputMode : 1;
	
	// 이동할 위치 캐싱
	FVector CachedDestination;
	
	// 조종 중인 캐릭터 캐싱
	UPROPERTY(Transient)
	TObjectPtr<ABaseCharacter> ControlledBaseChar;
	
	// 네트워크 부하 방지를 위한 타이머 
	float LastRPCUpdateTime;

	// RPC 전송 간격 (0.1 = 초당 10번)
	const float RPCUpdateInterval = 0.1f;

	/*// Camera Comp Added to the pawn character
	void CreateAndAttachCamera(APawn* InPawn);*/

/// [김현수 추가분] - 시작
protected:
	// 상호작용 타겟 캐싱
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectER|Item")
	TObjectPtr<AActor> InteractionTarget;

	// 상호작용 거리 저장
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectER|Item")
	float InteractionTargetDistance = 200.f;

	// 거리 체크 로직 (PlayerTick에서 호출)
	void CheckInteractionDistance();

	// 기존 Move 함수를 확장하여 상호작용 판정 포함
	void ProcessMouseInteraction();

public:
	// UI 드래그 취소(월드로 드랍) 시 로컬에서 호출
	void RequestDropInventoryItemFromUI(int32 SlotIndex, const FVector2D& ScreenSpacePosition);

	UFUNCTION(Server, Reliable)
	void Server_DropInventoryItem(int32 SlotIndex, FVector_NetQuantize DropLocation);

	UPROPERTY(EditDefaultsOnly, Category = "Item|Drop")
	TSubclassOf<ABaseItemActor> DroppedItemActorClass;
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

	// [Asset Preloading]
	UFUNCTION(BlueprintCallable, Client, Reliable)
	void Client_StartPreload();

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_NotifyLoadComplete();

	UFUNCTION()
	void OnPreloadComplete();

	// 아이템 루팅 RPC

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_RequestPickup(class ABaseItemActor* Item);

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_BeginLoot(AActor* Actor);// 루팅 시작 요청

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_EndLoot(); // 루팅 종료 요청(또는 거리 벗어남)

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_TakeItem(ABaseBoxActor* Box, int32 SlotIndex); // 아이템 가져가기(핵심)

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_BeginLootFromActor(AActor* TargetActor);

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_TakeItemFromActor(const AActor* TargetActor, int32 SlotIndex);



	UFUNCTION(BlueprintCallable, Client, Reliable)
	void Client_OpenLootUI(const AActor* Box); // UI 열기(로컬 전용)

	UFUNCTION(BlueprintCallable, Client, Reliable)
	void Client_CloseLootUI(); // UI 닫기(로컬 전용)

	UFUNCTION(BlueprintCallable, Client, Reliable)
	void Client_OpenLoadingUI();

	UFUNCTION(BlueprintCallable, Client, Reliable)
	void Client_CloseLoadingUI();

	// 클라이언트가 캐릭터 선택창 진입 요청
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_RequestCharacterSelection();

	// 유저가 특정 캐릭터 버튼을 클릭했을 때 호출 (서버에 데이터 저장 요청)
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Character Selection")
	void Server_SelectCharacter(const TSoftObjectPtr<UCharacterData>& SelectedData);

	// 서버가 모든 클라이언트에게 캐릭터 선택 UI를 띄우라고 명령
	UFUNCTION(BlueprintCallable, Client, Reliable)
	void Client_ShowCharacterSelectionUI();

	// HUD BP에서 UI를 스왑하기 위해 사용할 이벤트
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnShowCharacterSelectionUI();

	// [텔레포트 관련]
	UFUNCTION(BlueprintCallable, Client, Reliable)
	void Client_OpenTeleportUI(AActor* TeleportActor);

	UFUNCTION(BlueprintCallable, Client, Reliable)
	void Client_CloseTeleportUI();

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_RequestTeleport(int32 RegionIndex);

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_BeginTeleportInteract(class UER_TeleportComponent* TeleportComp);
	// 박스 아이템 루팅 RPC 끝

	//	mpyi 추가분 _ UI SYSTEM
public:
	UFUNCTION()
	void setMainHud(UUI_MainHUD* InMainHUD) { MainHUD = InMainHUD; }

	UFUNCTION()
	void UI_RespawnStart(float RespawnTime);

	UFUNCTION(BlueprintCallable, Client, Reliable)
	void UI_KillCountUpdate(int32 KillCount);
	
	UFUNCTION(BlueprintCallable, Client, Reliable)
	void UI_DeathCountUpdate(int32 DeathCount);
	
	UFUNCTION(BlueprintCallable, Client, Reliable)
	void UI_AssistCountUpdate(int32 AssistCount);

	// 인벤토리 업데이트 핸들러
	UFUNCTION()
	void OnInventoryUpdated();
private:
	UPROPERTY()
	UUI_MainHUD* MainHUD;

	//

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

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> LoadingUIClass;

	// Teleport UI
	UPROPERTY(EditDefaultsOnly, Category = "ProjectER|UI")
	TSubclassOf<UUserWidget> TeleportUIClass;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> TeleportUIInstance;
	
	// 거리 측정을 위한 타겟 캐싱
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CurrentTeleportActor;

	// Currently bound loot component for automatic popup close
	TWeakObjectPtr<class ULootableComponent> BoundLootComponent;

	// Close loot widget locally and cleanup bindings
	void CloseLootWidgetLocal();

	// Handler for loot depleted delegate
	void HandleLootDepleted();

	// Add reference to curved world subsystem
	UPROPERTY()
	TObjectPtr<UCurvedWorldSubsystem> CurvedWorldSubsystem;//cahced subsystem
    

	void UseInventorySlot(int32 SlotIndex);


	// 사운드
	UPROPERTY(EditDefaultsOnly)
	USoundMix* SoundMix;

	UPROPERTY(EditAnywhere)
	TMap<EAudioType, USoundClass*> SoundClassMap;

	void SetSoundMix(EAudioType AudioType, float Volume);
};
