// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "WatchTagAbility.generated.h"

class USkillEffectDataAsset;

/**
 * 특정 GameplayTag(이벤트)를 감시하다가 조건 충족 시 SkillEffectDataAsset의 효과를 적용하는 패시브 어빌리티
 */
UCLASS()
class PROJECTER_API UWatchTagAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UWatchTagAbility();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	/** 감시할 이벤트 태그 */
	UPROPERTY(EditDefaultsOnly, Category = "WatchTag")
	FGameplayTag EventTagToWatch;

	/** 발동 시 적용할 이펙트 데이터 세트 */
	UPROPERTY(EditDefaultsOnly, Category = "WatchTag")
	TArray<TObjectPtr<USkillEffectDataAsset>> SkillEffectDataAssets;

	/** 시전 대상(Instigator)에게 적용할지, 이벤트를 발생시킨 대상(Target)에게 적용할지 */
	UPROPERTY(EditDefaultsOnly, Category = "WatchTag")
	bool bApplyToTriggerInstigator = false;

	UFUNCTION()
	void OnEventReceived(FGameplayEventData Payload);

private:
	void ApplySkillEffects(AActor* TargetActor);
};
