#include "Monster/StateTree/Task/STT_ActivateTargetSkill.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Monster/BaseMonster.h"

FSTT_ActivateTargetSkill::FSTT_ActivateTargetSkill()
{
	bShouldCallTick = false;
}

bool FSTT_ActivateTargetSkill::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(ActorHandle);
	return true;
}

const UStruct* FSTT_ActivateTargetSkill::GetInstanceDataType() const
{
	return FInstanceDataType::StaticStruct();
}

EStateTreeRunStatus FSTT_ActivateTargetSkill::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	AActor& Actor = Context.GetExternalData(ActorHandle);
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	UAbilitySystemComponent* ASC = 
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(&Actor);

	for (FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.DynamicAbilityTags.HasTagExact(InstanceData.AbilityTag))
		{
			if (ASC->TryActivateAbility(Spec.Handle))
			{
				FGameplayEventData Payload;
				Payload.Instigator = &Actor;
				if (ABaseMonster* Monster = Cast<ABaseMonster>(&Actor))
				{
					Payload.Target = Monster->GetTargetPlayer();

					FGameplayAbilityTargetDataHandle TargetDataHandle = 
						UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(Monster->GetTargetPlayer());
					Payload.TargetData = TargetDataHandle;
						
				}

				UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(&Actor, InstanceData.EventTag, Payload);
			
				return EStateTreeRunStatus::Running;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Try Activate Skill Fail"));

				return EStateTreeRunStatus::Failed;
			}
			break;
		}
	}

	return EStateTreeRunStatus::Failed;
}

void FSTT_ActivateTargetSkill::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
}
