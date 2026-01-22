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

	// 스킬 입력 
	/* 입력 시, Input Data의 Gameplay Tag 전달 -> 어빌리티 실행 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilitiy", meta = (TitleProperty = "InputTag"))
	TArray<FInputData> AbilityInputAction;
};
