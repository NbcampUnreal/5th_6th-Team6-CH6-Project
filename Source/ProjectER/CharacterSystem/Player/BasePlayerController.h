#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h" 
#include "BasePlayerController.generated.h"

class UInputMappingContext;
class UInputConfig;
class UInputAction;
class UPathFollowingComponent;

struct FInputActionValue;

UCLASS()
class PROJECTER_API ABasePlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ABasePlayerController();
	
protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;
	
	// 우클릭 이동 / 공격
	void OnMoveTriggered();
	void OnMoveReleased();
	void MoveToMouseCursor();
	
	// 키 입력 시, 어빌리티 호출
	void AbilityInputTagPressed(FGameplayTag InputTag);
	void AbilityInputTagReleased(FGameplayTag InputTag);
	
protected:
	// NavMesh path 이동 
	UPROPERTY(VisibleDefaultsOnly, Category = AI)
	TObjectPtr<UPathFollowingComponent> PathFollowingComponent;
	
	// 입력 매핑 컨텍스트 (IMC)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	//  입력 설정 데이터 에셋 (InputConfig)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputConfig> InputConfig;
	
private:
	// 마우스 입력 키다운 플래그
	bool bIsMousePressed = false;
};
