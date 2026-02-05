// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/AbilityTask/AbilityTask_SendServerEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask.h"

UAbilityTask_SendServerEvent* UAbilityTask_SendServerEvent::SendServerEvent(UGameplayAbility* OwningAbility, FGameplayTag Tag)
{
    UAbilityTask_SendServerEvent* Task = NewAbilityTask<UAbilityTask_SendServerEvent>(OwningAbility);

	Task->EventTag = Tag;

    return Task;
}

void UAbilityTask_SendServerEvent::Activate()
{
    if (IsLocallyControlled())
    {
        Server_SendEvent(EventTag); // 1. 서버로 배달해! 라고 시킴
        EndTask(); // 2. "클라이언트" 태스크 종료 (OK)
    }
}

void UAbilityTask_SendServerEvent::Server_SendEvent_Implementation(FGameplayTag Tag)
{
    UE_LOG(LogTemp, Warning, TEXT("Server_SendEvent_Implementation"));
    if (Ability)
    {
        AActor* Avatar = Ability->GetAvatarActorFromActorInfo();

        if (Avatar)
        {
            UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Avatar, Tag, FGameplayEventData());
        }
    }

    EndTask();
}
