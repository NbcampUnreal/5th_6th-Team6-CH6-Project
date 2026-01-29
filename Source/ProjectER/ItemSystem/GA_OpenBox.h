#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Blueprint/UserWidget.h"
#include "GA_OpenBox.generated.h"

/**
 * UGA_OpenBox
 */
UCLASS()
class PROJECTER_API UGA_OpenBox : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_OpenBox();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "ProjectER|Design")
	float OpenTime = 1.5f;

	// 에디터에서 WBP_LootingPopup을 선택할 변수
	UPROPERTY(EditDefaultsOnly, Category = "ProjectER|UI")
	TSubclassOf<class UW_LootingPopup> LootWidgetClass;

	UFUNCTION()
	void OnFinishOpen();
};