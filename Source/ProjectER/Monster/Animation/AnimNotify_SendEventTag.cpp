#include "Monster/Animation/AnimNotify_SendEventTag.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Monster/BaseMonster.h"

void UAnimNotify_SendEventTag::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp || !MeshComp->GetOwner())
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Instigator = MeshComp->GetOwner();
	Payload.Target = MeshComp->GetOwner();

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(MeshComp->GetOwner(), EventTag, Payload);
}
