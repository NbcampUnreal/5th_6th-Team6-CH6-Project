// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/AnimNotify/AnimNotify_AddTagSkillCasting.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"

UAnimNotify_AddTagSkillCasting::UAnimNotify_AddTagSkillCasting()
{
	CastingTag = FGameplayTag::RequestGameplayTag(FName("Skill.Animation.Casting"));
}

void UAnimNotify_AddTagSkillCasting::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp) return;

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor) return;

	if (!MeshComp->GetWorld() || !MeshComp->GetWorld()->IsGameWorld()) return;

	// 이벤트 전송
	FGameplayEventData Payload;
	Payload.EventTag = CastingTag;
	Payload.Instigator = OwnerActor;
	Payload.Target = OwnerActor;
	Payload.EventMagnitude = EventMagnitude;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerActor, CastingTag, Payload);
}
