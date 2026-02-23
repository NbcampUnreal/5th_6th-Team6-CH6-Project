#include "Monster/Animation/AnimNotify_SendStateTreeEventTag.h"

#include "Components/StateTreeComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Monster/BaseMonster.h"

void UAnimNotify_SendStateTreeEventTag::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp || !MeshComp->GetOwner())
	{
		return;
	}

	ABaseMonster* BaseMonster = Cast<ABaseMonster>(MeshComp->GetOwner());
	if (!BaseMonster)
	{
		return;
	}

	BaseMonster->SendStateTreeEvent(EventTag);
}
