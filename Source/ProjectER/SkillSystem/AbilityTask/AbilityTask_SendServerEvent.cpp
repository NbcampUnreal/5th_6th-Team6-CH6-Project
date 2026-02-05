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
        UE_LOG(LogTemp, Warning, TEXT("Activate - Client: Sending RPC"));
        //Server_SendEvent(EventTag);

        // [선택] 클라이언트에서도 로컬 반응이 필요하다면 여기서 로컬용 이벤트를 쏠 수 있음
        // 만약 서버에서만 로직이 돌아가길 원한다면 여기서 EndTask를 하거나 기다림
    }
    else if (GetAvatarActor() && GetAvatarActor()->HasAuthority())
    {
        // 서버에서 생성된 경우: 서버는 클라이언트의 RPC를 기다려야 하므로 
        // 여기서 EndTask를 부르지 않고 대기합니다. (정상 동작)
        UE_LOG(LogTemp, Warning, TEXT("Activate - Server: Waiting for RPC"));
    }
    else
    {
        // 그 외 예외 상황(다른 클라이언트 등)에서는 태스크 종료
        EndTask();
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
