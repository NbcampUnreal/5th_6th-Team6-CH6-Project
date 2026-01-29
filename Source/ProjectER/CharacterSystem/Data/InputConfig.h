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
	
	// 이동 정지 명령
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Command")
	TObjectPtr<UInputAction> StopMove;

	// 스킬 입력 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilitiy", meta = (TitleProperty = "InputTag"))
	TArray<FInputData> AbilityInputAction;
};
