// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/AbilityTask/AbilityTask_WaitGameplayEventSyn.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "Engine/EngineTypes.h"
#include "Abilities/GameplayAbilityTargetTypes.h"

UAbilityTask_WaitGameplayEventSyn* UAbilityTask_WaitGameplayEventSyn::WaitEventClientToServer(UGameplayAbility* OwningAbility, FGameplayTag EventTag)
{
    UAbilityTask_WaitGameplayEventSyn* MyObj = NewAbilityTask<UAbilityTask_WaitGameplayEventSyn>(OwningAbility);
    MyObj->TagToWait = EventTag;
    return MyObj;
}

void UAbilityTask_WaitGameplayEventSyn::Activate()
{
    UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
    if (!ASC || !Ability) return;

    // 공식 ActorInfo를 사용하여 로컬 컨트롤 여부를 확실하게 판단 (PlayerState 소유 대응)
    const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
    const bool bIsLocallyControlled = ActorInfo ? ActorInfo->IsLocallyControlledPlayer() : false;
    const bool bIsAuthority = ActorInfo ? ActorInfo->IsNetAuthority() : false;

    if (bIsAuthority)
    {
        UE_LOG(LogTemp, Warning, TEXT("WaitEventTask [%s] on [%s]: Registering Server Local Callback"), *TagToWait.ToString(), *GetOwnerActor()->GetName());
        ASC->GenericGameplayEventCallbacks.FindOrAdd(TagToWait).AddUObject(this, &UAbilityTask_WaitGameplayEventSyn::OnEventTriggeredOnServer);

        // 원격 클라이언트 플레이어인 경우에만 TargetData 대기
        if (!bIsLocallyControlled)
        {
            UE_LOG(LogTemp, Warning, TEXT("WaitEventTask [%s]: Registering TargetData Callback (Remote Player)"), *TagToWait.ToString());
            FGameplayAbilitySpecHandle SpecHandle = GetAbilitySpecHandle();
            FPredictionKey ActivationKey = GetActivationPredictionKey();
            TargetDataDelegateHandle = ASC->AbilityTargetDataSetDelegate(SpecHandle, ActivationKey).AddUObject(this, &UAbilityTask_WaitGameplayEventSyn::OnTargetDataReplicated);
            ASC->CallReplicatedTargetDataDelegatesIfSet(SpecHandle, ActivationKey);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("WaitEventTask [%s]: Registering Client Local Callback"), *TagToWait.ToString());
        ASC->GenericGameplayEventCallbacks.FindOrAdd(TagToWait).AddUObject(this, &UAbilityTask_WaitGameplayEventSyn::OnEventTriggeredOnClient);
    }

    Super::Activate();
}

void UAbilityTask_WaitGameplayEventSyn::OnEventTriggeredOnServer(const FGameplayEventData* EventData)
{
    UE_LOG(LogTemp, Warning, TEXT("WaitEventTask [%p] on [%s] - Tag [%s]: Server Local Event Detected. bClientDataPending: %d"), 
        this, *GetOwnerActor()->GetName(), *TagToWait.ToString(), bClientDataPending);

    if (bClientDataPending)
    {
        UE_LOG(LogTemp, Display, TEXT("WaitEventTask [%s]: Server Local Event Ignored (Already handled by Client Nudge)"), *TagToWait.ToString());
        bClientDataPending = false;
        return;
    }

    bHandledByServer = true;
    OnEventReceived.Broadcast(*EventData);
}

void UAbilityTask_WaitGameplayEventSyn::OnEventTriggeredOnClient(const FGameplayEventData* EventData)
{
    UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
    if (IsValid(ASC) == false) return;

    UE_LOG(LogTemp, Warning, TEXT("WaitEventTask [%s]: Client Local Event Detected. Sending Nudge to Server."), *TagToWait.ToString());

    // 실제 데이터를 TargetData에 담아 전송
    FGameplayAbilityTargetDataHandle DataHandle;
    FGameplayAbilityTargetData_ActorArray* NewData = new FGameplayAbilityTargetData_ActorArray();
    if (EventData->Instigator) NewData->TargetActorArray.Add(const_cast<AActor*>(EventData->Instigator.Get()));
    if (EventData->Target) NewData->TargetActorArray.Add(const_cast<AActor*>(EventData->Target.Get()));
    DataHandle.Add(NewData);

    FScopedPredictionWindow ScopedPrediction(ASC);
    ASC->CallServerSetReplicatedTargetData(GetAbilitySpecHandle(), GetActivationPredictionKey(), DataHandle, TagToWait, GetActivationPredictionKey());

    OnEventReceived.Broadcast(*EventData);
}

void UAbilityTask_WaitGameplayEventSyn::OnTargetDataReplicated(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ActivationTag)
{
    UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
    if (IsValid(ASC) == false) return;
    if (ActivationTag != TagToWait) return;

    UE_LOG(LogTemp, Warning, TEXT("WaitEventTask [%s]: Client Nudge Received on Server. bHandledByServer: %d"), *TagToWait.ToString(), bHandledByServer);

    ASC->ConsumeClientReplicatedTargetData(GetAbilitySpecHandle(), GetActivationPredictionKey());

    FGameplayAbilitySpecHandle SpecHandle = GetAbilitySpecHandle();
    FPredictionKey ActivationKey = GetActivationPredictionKey();
    TargetDataDelegateHandle = ASC->AbilityTargetDataSetDelegate(SpecHandle, ActivationKey).AddUObject(this, &UAbilityTask_WaitGameplayEventSyn::OnTargetDataReplicated);

    if (bHandledByServer)
    {
        UE_LOG(LogTemp, Display, TEXT("WaitEventTask [%s]: Client Nudge Ignored (Already handled by Server Local)"), *TagToWait.ToString());
        bHandledByServer = false;
        return;
    }

    bClientDataPending = true;

    // 수신한 TargetData에서 데이터 복구
    FGameplayEventData ReconstructedData;
    ReconstructedData.EventTag = TagToWait;
    if (DataHandle.Num() > 0)
    {
        const FGameplayAbilityTargetData_ActorArray* ActorData = static_cast<const FGameplayAbilityTargetData_ActorArray*>(DataHandle.Get(0));
        if (ActorData && ActorData->TargetActorArray.Num() >= 2)
        {
            ReconstructedData.Instigator = ActorData->TargetActorArray[0].Get();
            ReconstructedData.Target = ActorData->TargetActorArray[1].Get();
        }
    }

    OnEventReceived.Broadcast(ReconstructedData);
}

void UAbilityTask_WaitGameplayEventSyn::OnDestroy(bool bInOwnerFinished)
{
    UE_LOG(LogTemp, Warning, TEXT("WaitEventTask [%s] on [%s]: Task Destroyed"), *TagToWait.ToString(), *GetOwnerActor()->GetName());
    UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
    if (ASC)
    {
        ASC->GenericGameplayEventCallbacks.FindOrAdd(TagToWait).RemoveAll(this);
        ASC->AbilityTargetDataSetDelegate(GetAbilitySpecHandle(), GetActivationPredictionKey()).Remove(TargetDataDelegateHandle);
    }
    Super::OnDestroy(bInOwnerFinished);
}
