#pragma once

#include "CoreMinimal.h"
#include "Monster/GAS/GA/GA_MonsterState.h"
#include "GA_MonsterState_Attack.generated.h"

UCLASS()
class PROJECTER_API UGA_MonsterState_Attack : public UGA_MonsterState
{
	GENERATED_BODY()
public:
	UGA_MonsterState_Attack();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle, 
		const FGameplayAbilityActorInfo* ActorInfo, 
		const FGameplayAbilityActivationInfo ActivationInfo, 
		const FGameplayEventData* TriggerEventData
	) override;

protected:
	UFUNCTION()
	void OnAttackHitEventReceived(FGameplayEventData Payload);

	UPROPERTY(EditDefaultsOnly, Category = "MonsterState")
	TSubclassOf<UGameplayEffect> DamageEffectClass;
};
