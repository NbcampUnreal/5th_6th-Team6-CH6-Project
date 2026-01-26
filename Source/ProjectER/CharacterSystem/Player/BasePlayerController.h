#pragma once

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
};
