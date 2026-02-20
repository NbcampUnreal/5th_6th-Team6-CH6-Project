#include "CharacterSystem/Animation/AnimNotify_SendGameplayEvent.h"

#include "AbilitySystemBlueprintLibrary.h" 
#include "AbilitySystemComponent.h"

UAnimNotify_SendGameplayEvent::UAnimNotify_SendGameplayEvent()
{
}

void UAnimNotify_SendGameplayEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	
	if (!MeshComp) return;
	
	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor) return;

	if (!MeshComp->GetWorld() || !MeshComp->GetWorld()->IsGameWorld()) return;

	// 이벤트 전송
	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Instigator = OwnerActor;
	Payload.Target = OwnerActor;
	Payload.EventMagnitude = EventMagnitude;

#if WITH_EDITOR
	// UE_LOG(LogTemp, Warning, TEXT("[AnimNotify] Activate SendGamePlayEvent.!!!"));
#endif
	
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerActor, EventTag, Payload);
}