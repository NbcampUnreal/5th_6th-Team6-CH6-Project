#pragma once

#include "CoreMinimal.h"
#include "Monster/GAS/GA/GA_MonsterState.h"
#include "GA_MonsterState_Idle.generated.h"

UCLASS()
class PROJECTER_API UGA_MonsterState_Idle : public UGA_MonsterState
{
	GENERATED_BODY()
	
	UGA_MonsterState_Idle();

protected:

	virtual void OnGiveAbility(
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilitySpec& Spec
	) override;

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

};
