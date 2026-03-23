#include "Monster/StateTree/Task/STT_ActivateDirectionSkill.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Monster/BaseMonster.h"

FSTT_ActivateDirectionSkill::FSTT_ActivateDirectionSkill()
{
	bShouldCallTick = false;
}

bool FSTT_ActivateDirectionSkill::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(ActorHandle);
	return true;
}

const UStruct* FSTT_ActivateDirectionSkill::GetInstanceDataType() const
{
	return FInstanceDataType::StaticStruct();
}

EStateTreeRunStatus FSTT_ActivateDirectionSkill::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
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

						FVector StartLocation = Actor.GetActorLocation();
						FVector TargetLocation = TargetActor->GetActorLocation();
						FVector Direction = (TargetLocation - StartLocation).GetSafeNormal();
						FVector AimPoint = StartLocation + Direction;
						
						FHitResult HitResult;
						HitResult.Location = AimPoint;

						Payload.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(HitResult);

						UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(&Actor, InstanceData.EventTag, Payload);
						
						FGameplayCueParameters Params;
						Params.Instigator = &Actor;
						Params.Location = AimPoint;
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

void FSTT_ActivateDirectionSkill::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
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
