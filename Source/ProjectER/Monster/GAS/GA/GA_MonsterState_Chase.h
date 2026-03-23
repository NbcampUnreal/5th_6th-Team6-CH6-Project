#pragma once

#include "CoreMinimal.h"
#include "Monster/GAS/GA/GA_MonsterState.h"
#include "AIController.h"
#include "GA_MonsterState_Chase.generated.h"

UCLASS()
class PROJECTER_API UGA_MonsterState_Chase : public UGA_MonsterState
{
	GENERATED_BODY()

public:
	UGA_MonsterState_Chase();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle, 
		const FGameplayAbilityActorInfo* ActorInfo, 
		const FGameplayAbilityActivationInfo ActivationInfo, 
		const FGameplayEventData* TriggerEventData
	) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle, 
		const FGameplayAbilityActorInfo* ActorInfo, 
		const FGameplayAbilityActivationInfo ActivationInfo, 
		bool bReplicateEndAbility, 
		bool bWasCancelled
	) override;

private:
	UFUNCTION()
	void OnMoveFinished(FAIRequestID RequestID, EPathFollowingResult::Type Result);
};
