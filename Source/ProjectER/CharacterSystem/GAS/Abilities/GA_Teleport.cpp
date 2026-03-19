#include "GA_Teleport.h"
#include "CharacterSystem/Character/BaseCharacter.h"
#include "GameModeBase/GameMode/ER_InGameMode.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Engine/World.h"

UGA_Teleport::UGA_Teleport()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
}

void UGA_Teleport::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	UE_LOG(LogTemp, Log, TEXT("[GA_Teleport] ActivateAbility"));
	if (!HasAuthority(&ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	if (TriggerEventData && ActorInfo->AvatarActor.IsValid())
	{
		TargetRegionIndex = static_cast<int32>(TriggerEventData->EventMagnitude);

		// 지정된 시간 대기 태스크 생성 및 실행
		UAbilityTask_WaitDelay* WaitDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, TeleportDelayTime);
		if (WaitDelayTask)
		{
			WaitDelayTask->OnFinish.AddDynamic(this, &UGA_Teleport::OnDelayFinish);
			WaitDelayTask->ReadyForActivation();
		}
		else
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		}
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UGA_Teleport::OnDelayFinish()
{
	// 어빌리티가 아직 활성화 상태인지(취소되지 않았는지) 확인
	if (IsActive())
	{
		ABaseCharacter* Char = Cast<ABaseCharacter>(GetAvatarActorFromActorInfo());
		if (Char)
		{
			if (AER_InGameMode* GM = Cast<AER_InGameMode>(GetWorld()->GetAuthGameMode()))
			{
				GM->RequestTeleportToRegion(Char, TargetRegionIndex);
			}
		}

		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}
