#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Teleport.generated.h"

/**
 * Gameplay Ability triggered by a Gameplay Event to teleport the player.
 * The destination RegionIndex is extracted from EventData->EventMagnitude.
 */
UCLASS()
class PROJECTER_API UGA_Teleport : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_Teleport();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UFUNCTION()
	void OnDelayFinish();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Teleport|State")
	bool IsCharacterDead() const;

	// 텔레포트 시전 대기 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Teleport")
	float TeleportDelayTime = 1.0f;

private:
	int32 TargetRegionIndex;
};
