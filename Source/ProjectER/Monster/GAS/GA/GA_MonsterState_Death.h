#pragma once

#include "CoreMinimal.h"
#include "Monster/GAS/GA/GA_MonsterState.h"
#include "GA_MonsterState_Death.generated.h"

UCLASS()
class PROJECTER_API UGA_MonsterState_Death : public UGA_MonsterState
{
	GENERATED_BODY()
	
	UGA_MonsterState_Death();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;
};
