#include "Monster/StateTree/Consideration/STCons_AttackCountUtility.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"
#include "AbilitySystemComponent.h"

#include "Monster/BaseMonster.h"

FSTCons_AttackCountUtility::FSTCons_AttackCountUtility()
{
}

bool FSTCons_AttackCountUtility::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(ActorHandle);
	return true;
}

const UStruct* FSTCons_AttackCountUtility::GetInstanceDataType() const
{
	return FInstanceDataType::StaticStruct();
}

float FSTCons_AttackCountUtility::GetScore(FStateTreeExecutionContext& Context) const
{
	AActor& Actor = Context.GetExternalData(ActorHandle);
	if (!IsValid(&Actor))
	{
		return -1.f;
	}

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	ABaseMonster* Monster = Cast<ABaseMonster>(&Actor);

	if (!IsValid(Monster))
	{
		return -1.f;
	}

	if (InstanceData.AttackCountThreshold <= Monster->GetAttackCount())
	{
		UAbilitySystemComponent* ASC = Monster->GetAbilitySystemComponent();
		if (ASC && ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag("Cooldown.Skill.R")))
		{
			return 0.f;
		}

		return 1.f;
	}


	return 0.f;
}
