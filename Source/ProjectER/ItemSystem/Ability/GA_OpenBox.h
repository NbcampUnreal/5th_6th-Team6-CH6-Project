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
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//virtual void CancelAbility()

protected:

	// 거리 체크 타이머 시작
	void StartDistanceCheck(const FGameplayAbilityActorInfo* ActorInfo);

	// 거리 체크 타이머 종료
	void StopDistanceCheck();

	// 거리 체크
	UFUNCTION()
	void TickDistanceCheck();


	UPROPERTY(EditDefaultsOnly, Category = "ProjectER|Design")
	float OpenTime = 1.5f;

	// 박스와의 최대 거리
	UPROPERTY(EditDefaultsOnly, Category = "OpenBox")
	float MaxLootDistance = 150.f;

	// 거리를 체크할 tick 시간
	UPROPERTY(EditDefaultsOnly, Category = "OpenBox")
	float DistanceCheckInterval = 0.1f;

	// 체크할 박스 캐싱
	UPROPERTY()
	TObjectPtr<const AActor> TargetBox;

	// 타이머
	FTimerHandle DistanceCheckTimer;


};