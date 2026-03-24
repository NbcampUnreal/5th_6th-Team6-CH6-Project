#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Monster/Data/MonsterDataAsset.h"
#include "GA_MonsterState.generated.h"

USTRUCT(BlueprintType)
struct FMonsterStateInitData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "MonsterState|Montage")
	EMonsterMontageType MontageType = EMonsterMontageType::None;

	UPROPERTY(EditAnywhere, Category = "MonsterState|Tag")
	FGameplayTagContainer MonsterAssetTags;

	UPROPERTY(EditAnywhere, Category = "MonsterState|Tag")
	FGameplayTag SoundCueTag;

	UPROPERTY(EditAnywhere, Category = "MonsterState|Tag")
	FGameplayTag NiagaraCueTag;

	UPROPERTY(EditAnywhere, Category = "MonsterState|Tag")
	FGameplayTag WaitTag;
};

UCLASS()
class PROJECTER_API UGA_MonsterState : public UGameplayAbility
{
	GENERATED_BODY()

public:

	UGA_MonsterState();

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


	UFUNCTION()
	virtual void OnMontageCompleted();

	UFUNCTION()
	virtual void OnMontageBlendIn();

	UFUNCTION()
	virtual void OnMontageBlendOut();

	UFUNCTION()
	virtual void OnMontageInterrupt();

	UFUNCTION()
	virtual void OnMontageCancel();



	UFUNCTION()
	virtual void OnTagRemoved();

	UPROPERTY(EditAnywhere, Category = "MonsterState")
	bool bIsUseWaitTag = false;



	UPROPERTY(EditAnywhere, Category = "MonsterState")
	FMonsterStateInitData StateInitData;
};
