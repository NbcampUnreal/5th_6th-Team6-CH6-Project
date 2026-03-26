// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_WaitGameplayEventSyn.generated.h"

/**
 * 
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWaitEventClientToServerDelegate, FGameplayEventData, EventData);

UCLASS()
class PROJECTER_API UAbilityTask_WaitGameplayEventSyn : public UAbilityTask
{
	GENERATED_BODY()
public:
    // 블루프린트에서 호출할 노드 이름
    UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
    static UAbilityTask_WaitGameplayEventSyn* WaitEventClientToServer(UGameplayAbility* OwningAbility, FGameplayTag EventTag);

    UPROPERTY(BlueprintAssignable)
    FWaitEventClientToServerDelegate OnEventReceived;

    // 태스크 시작 시 호출
    virtual void Activate() override;

protected:
    FGameplayTag TagToWait;
    FDelegateHandle TargetDataDelegateHandle;

    // 서버가 로컬에서 이벤트를 처리했는지 여부
    bool bHandledByServer = false;

    // 클라이언트 신호가 먼저 도착해서 처리됐는지 여부
    bool bClientDataPending = false;

    // 서버(Authority)에서 로컬 태그 이벤트를 감지했을 때 호출
    void OnEventTriggeredOnServer(const FGameplayEventData* EventData);

    // 클라이언트에서 로컬 태그 이벤트를 감지했을 때 호출
    void OnEventTriggeredOnClient(const FGameplayEventData* EventData);

    // 서버에서 클라이언트가 보낸 TargetData를 받았을 때 실행될 함수
    void OnTargetDataReplicated(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ActivationTag);

    virtual void OnDestroy(bool bInOwnerFinished) override;
};
