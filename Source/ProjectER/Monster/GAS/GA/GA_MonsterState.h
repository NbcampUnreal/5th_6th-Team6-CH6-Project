#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Monster/Data/MonsterDataAsset.h"
#include "GA_MonsterState.generated.h"

USTRUCT(BlueprintType)
struct FMonsterStateInitData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Montage")
	FGameplayTagContainer MonsterAssetTags;

	UPROPERTY(EditAnywhere, Category = "Montage")
	EMonsterMontageType MontageType = EMonsterMontageType::None;

	UPROPERTY(EditAnywhere, Category = "Cue")
	FGameplayTag SoundCueTag;

	UPROPERTY(EditAnywhere, Category = "Cue")
	FGameplayTag NiagaraCueTag;

	UPROPERTY(EditAnywhere, Category = "Task")
	FGameplayTag WaitTag;
};

UCLASS()
class PROJECTER_API UGA_MonsterState : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_MonsterState();

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

	virtual void OnGiveAbility(
		const FGameplayAbilityActorInfo* ActorInfo, 
		const FGameplayAbilitySpec& Spec
	) override;


protected:
	UFUNCTION()
	void OnTagRemoved();

	UFUNCTION()
	void OnMontageFinished();

protected:
	UPROPERTY(EditAnywhere, Category = "MonsterState")
	FMonsterStateInitData StateInitData;

	bool bIsWaitTag = false;
};
