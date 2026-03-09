#include "Monster/Animation/AnimNotify_SendStateTreeEventTag.h"

#include "Components/StateTreeComponent.h"
#include "Monster/BaseMonster.h"

void UAnimNotify_SendStateTreeEventTag::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp || !MeshComp->GetWorld()->IsGameWorld())
	{
		return;
	}
	if (!MeshComp || !MeshComp->GetOwner())
	{
		UE_LOG(LogTemp, Warning, TEXT("UAnimNotify_SendStateTreeEventTag::Notify : Not Mesh"));
		return;
	}
	if (!MeshComp->GetOwner()->HasAuthority())
	{
		return;
	}
	ABaseMonster* Monster = Cast<ABaseMonster>(MeshComp->GetOwner());
	if (IsValid(Monster) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("UAnimNotify_SendStateTreeEventTag::Notify : Not Monster"));
		return;
	}
	//UE_LOG(LogTemp, Warning, TEXT("Animation : %s"), *Animation->GetName());
	Monster->SendStateTreeEvent(EventTag);
}
