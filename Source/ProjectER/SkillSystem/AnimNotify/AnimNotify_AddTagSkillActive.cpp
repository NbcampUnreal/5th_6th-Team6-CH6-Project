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
	UE_LOG(LogTemp, Log, TEXT("UAnimNotify_AddTagSkillActive::Notify"));

	if (!MeshComp || !MeshComp->GetOwner() || !ActiveTag.IsValid()) return;

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp->GetOwner());
	//같은 태그가 여러개 있어도 해당 태그를 1개로 설정
	if (ASC) {
		UE_LOG(LogTemp, Log, TEXT("UAnimNotify_AddTagSkillActive::Notify::ASC"));
		ASC->AddLooseGameplayTag(ActiveTag, 1);
	}
	else {
		UE_LOG(LogTemp, Log, TEXT("UAnimNotify_AddTagSkillActive::Notify::ASC is Null"));
	}
}
