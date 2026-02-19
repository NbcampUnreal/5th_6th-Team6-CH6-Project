// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/AnimNotify/AnimNotify_AddTagSkillActive.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"

UAnimNotify_AddTagSkillActive::UAnimNotify_AddTagSkillActive()
{
	ActiveTag = FGameplayTag::RequestGameplayTag(FName("Skill.Animation.Active"));
}

void UAnimNotify_AddTagSkillActive::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp) return;

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor) return;

	if (!MeshComp->GetWorld() || !MeshComp->GetWorld()->IsGameWorld()) return;

	// 이벤트 전송
	FGameplayEventData Payload;
	Payload.EventTag = ActiveTag;
	Payload.Instigator = OwnerActor;
	Payload.Target = OwnerActor;
	Payload.EventMagnitude = EventMagnitude;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerActor, ActiveTag, Payload);
}
