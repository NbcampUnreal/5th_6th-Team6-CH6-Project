// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/AbilityTask/AbilityTask_WaitGameplayEventSyn.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"

UAbilityTask_WaitGameplayEventSyn* UAbilityTask_WaitGameplayEventSyn::WaitEventClientToServer(UGameplayAbility* OwningAbility, FGameplayTag EventTag)
{
    UAbilityTask_WaitGameplayEventSyn* MyObj = NewAbilityTask<UAbilityTask_WaitGameplayEventSyn>(OwningAbility);
    MyObj->TagToWait = EventTag;
    return MyObj;
}

void UAbilityTask_WaitGameplayEventSyn::Activate()
{
    UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
    if (!ASC) return;

    if (IsPredictingClient() || IsLocallyControlled())
    {
        ASC->GenericGameplayEventCallbacks.FindOrAdd(TagToWait).AddUObject(this, &UAbilityTask_WaitGameplayEventSyn::OnClientEventTriggered);
    }
    else
    {
        FGameplayAbilitySpecHandle SpecHandle = GetAbilitySpecHandle();
        FPredictionKey ActivationKey = GetActivationPredictionKey();
        TargetDataDelegateHandle = ASC->AbilityTargetDataSetDelegate(SpecHandle, ActivationKey).AddUObject(this, &UAbilityTask_WaitGameplayEventSyn::OnTargetDataReplicated);
        ASC->CallReplicatedTargetDataDelegatesIfSet(SpecHandle, ActivationKey);
    }

    Super::Activate();
}

void UAbilityTask_WaitGameplayEventSyn::OnClientEventTriggered(const FGameplayEventData* EventData)
{
    UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
    if (IsValid(ASC) == false) return;

    if (IsPredictingClient())
    {
        FGameplayAbilityTargetDataHandle DataHandle;
        FScopedPredictionWindow ScopedPrediction(ASC);
        ASC->CallServerSetReplicatedTargetData(GetAbilitySpecHandle(), GetActivationPredictionKey(), DataHandle, TagToWait, GetActivationPredictionKey());
    }

    OnEventReceived.Broadcast(*EventData);
}

void UAbilityTask_WaitGameplayEventSyn::OnTargetDataReplicated(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ActivationTag)
{
    UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
    if (IsValid(ASC) == false) return;
    if (ActivationTag != TagToWait) return;

    ASC->ConsumeClientReplicatedTargetData(GetAbilitySpecHandle(), GetActivationPredictionKey());
    FGameplayEventData DummyData;
    DummyData.EventTag = TagToWait;
    OnEventReceived.Broadcast(DummyData);
}

void UAbilityTask_WaitGameplayEventSyn::OnDestroy(bool bInOwnerFinished)
{
    UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
    if (ASC)
    {
        ASC->GenericGameplayEventCallbacks.FindOrAdd(TagToWait).RemoveAll(this);
        ASC->AbilityTargetDataSetDelegate(GetAbilitySpecHandle(), GetActivationPredictionKey()).Remove(TargetDataDelegateHandle);
    }
    Super::OnDestroy(bInOwnerFinished);
}
