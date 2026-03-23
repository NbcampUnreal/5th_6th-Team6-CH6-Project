#include "Monster/StateTree/Task/STT_ActivateGroundSkill.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Monster/BaseMonster.h"

FSTT_ActivateGroundSkill::FSTT_ActivateGroundSkill()
{
	bShouldCallTick = false;
}

bool FSTT_ActivateGroundSkill::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(ActorHandle);
	return true;
}

const UStruct* FSTT_ActivateGroundSkill::GetInstanceDataType() const
{
	return FInstanceDataType::StaticStruct();
}

EStateTreeRunStatus FSTT_ActivateGroundSkill::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
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
					AActor* TargetActor = Monster->GetTargetPlayer();
					if (IsValid(TargetActor))
					{
						Payload.Target = TargetActor;

						FVector TargetLocation = TargetActor->GetActorLocation();
						FHitResult HitResult;
						HitResult.Location = TargetLocation;

						Payload.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(HitResult);

						UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(&Actor, InstanceData.EventTag, Payload);

						FGameplayCueParameters Params;
						Params.Instigator = &Actor;
						Params.Location = TargetLocation;
						ASC->AddGameplayCue(InstanceData.GameplayCueTag, Params);
						
						return EStateTreeRunStatus::Running;
					}
				}
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

void FSTT_ActivateGroundSkill::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	AActor& Actor = Context.GetExternalData(ActorHandle);
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(&Actor);

	if (ASC && InstanceData.GameplayCueTag.IsValid())
	{
		ASC->RemoveGameplayCue(InstanceData.GameplayCueTag);
	}
}
