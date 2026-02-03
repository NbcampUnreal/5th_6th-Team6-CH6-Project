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

	if (!MeshComp || !MeshComp->GetOwner() || !CastingTag.IsValid()) return;

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp->GetOwner());
	//같은 태그가 여러개 있어도 해당 태그를 1개로 설정
	if (ASC) ASC->AddLooseGameplayTag(CastingTag, 1);
}
