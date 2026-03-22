#include "Monster/StateTree/Task/STT_AttackRangeCheck.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"

#include "Monster/BaseMonster.h"
#include "AbilitySystemComponent.h"
#include "Monster/GAS/AttributeSet/BaseMonsterAttributeSet.h"


FSTT_AttackRangeCheck::FSTT_AttackRangeCheck()
{
	bShouldCallTick = false;
}

bool FSTT_AttackRangeCheck::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(ActorHandle);
	return true;
}

const UStruct* FSTT_AttackRangeCheck::GetInstanceDataType() const
{
	return FInstanceDataType::StaticStruct();
}

EStateTreeRunStatus FSTT_AttackRangeCheck::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	AActor& Actor = Context.GetExternalData(ActorHandle);
	FAttackRangeCheckData& InstanceData = Context.GetInstanceData(*this);

	if (ABaseMonster* Monster = Cast<ABaseMonster>(&Actor))
	{
		AActor* Target = Monster->GetTargetPlayer();
		if (IsValid(Target))
		{
			float AttackRange = 0.f;
			AttackRange = Monster->GetAttributeSet()->GetAttackRange();
			const float DistanceSq = FVector::DistSquared(Target->GetActorLocation(), Actor.GetActorLocation());
			const float RangeSq = AttackRange * AttackRange;

			if (DistanceSq <= RangeSq)
			{
				Monster->SendStateTreeEvent(Monster->GetMonsterTags().AttackEventTag);
				return EStateTreeRunStatus::Running;
			}
			else
			{
				Monster->SendStateTreeEvent(Monster->GetMonsterTags().TargetOnEventTag);
				return EStateTreeRunStatus::Running;
			}
		}
	}

	return EStateTreeRunStatus::Failed;
}

void FSTT_AttackRangeCheck::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
}
