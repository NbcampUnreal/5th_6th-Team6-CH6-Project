#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "InputConfig.generated.h"

class UInputAction;

USTRUCT(BlueprintType)
struct FInputData
{
	GENERATED_BODY()
	
	// Input Action
	UPROPERTY(EditDefaultsOnly)
	const UInputAction* InputAction = nullptr;
	
	// Gameplay Tag
	UPROPERTY(EditDefaultsOnly)
	FGameplayTag InputTag;
};

UCLASS()
class PROJECTER_API UInputConfig : public UDataAsset
{
	GENERATED_BODY()
	
public:
	// 마우스 우클릭 (이동/공격)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MouseClick")
	TObjectPtr<UInputAction> InputMove;

	//마우스 오른쪽클릭(임시)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MouseClick")
	TObjectPtr<UInputAction> InputCancel;

	// 마우스 왼쪽클릭
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MouseClick")
	TObjectPtr<UInputAction> InputConfirm;
	
	// 공격 명령 (A키)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Command")
	TObjectPtr<UInputAction> InputAttack;
	
	// 이동 정지 명령
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Command")
	TObjectPtr<UInputAction> StopMove;

	// 스킬 입력 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilitiy", meta = (TitleProperty = "InputTag"))
	TArray<FInputData> AbilityInputAction;



	//Camera Control input
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UInputAction> InputCameraPanX;//(A/D, Left/Right Arrow)

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UInputAction> InputCameraPanY;//(W/S, Up/Down Arrow)
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UInputAction> InputCameraToggle;//Toggle

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UInputAction> InputCameraHold;//Hold


	// 아이템 사용

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "item")
	TObjectPtr<UInputAction> UseItem1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "item")
	TObjectPtr<UInputAction> UseItem2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "item")
	TObjectPtr<UInputAction> UseItem3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "item")
	TObjectPtr<UInputAction> UseItem4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "item")
	TObjectPtr<UInputAction> UseItem5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "item")
	TObjectPtr<UInputAction> UseItem6;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "item")
	TObjectPtr<UInputAction> UseItem7;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "item")
	TObjectPtr<UInputAction> UseItem8;

	// 아이템 조합키 (Z)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crafting")
	TObjectPtr<UInputAction> InputCraft;

	// 현황판 온오프용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UInputAction> ScoreBoardKey;

	// 채팅 온오프용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UInputAction> ChatEnterKey;
};
