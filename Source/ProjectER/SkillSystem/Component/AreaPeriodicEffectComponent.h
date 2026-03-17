// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AreaPeriodicEffectComponent.generated.h"

/**
 * 주기적으로 특정 타겟들에게 효과를 적용하도록 이벤트를 발생시키는 컴포넌트입니다.
 * 타이머와 타겟 목록 관리라는 '엔진' 역할만 수행합니다.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAreaPeriodicTrigger, const TArray<AActor*>&, Targets);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTER_API UAreaPeriodicEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAreaPeriodicEffectComponent();

	/** 주기적 효과의 기본 데이터를 설정합니다. (실행은 하지 않음) */
	void SetupPeriodicEffect(float InPeriod, bool bInApplyImmediately);

	/** 설정된 데이터를 바탕으로 주기적 트리거를 시작합니다. (BeginPlay 이후 권장) */
	void StartPeriodicTrigger();

	/** 주기적 트리거를 중단합니다. */
	void StopPeriodicTrigger();

	/** 범위 내 타겟을 추가합니다. (액터의 OverlapBegin에서 호출) */
	void AddTarget(AActor* InTarget);

	/** 범위 내 타겟을 제거합니다. (액터의 OverlapEnd에서 호출) */
	void RemoveTarget(AActor* InTarget);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 타이머에 의해 호출되는 내부 트리거 함수 */
	void OnTimerTrigger();

public:
	/** 주기마다 실행될 실제 로직을 연결할 델리게이트 */
	UPROPERTY(BlueprintAssignable, Category = "Periodic")
	FOnAreaPeriodicTrigger OnAreaPeriodicTrigger;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Periodic")
	float Period = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Periodic")
	bool bApplyImmediately = true;

	/** 현재 범위 내에 있는 유효한 타겟 목록 */
	UPROPERTY()
	TSet<TObjectPtr<AActor>> CurrentTargets;

	FTimerHandle PeriodicTimerHandle;
};
