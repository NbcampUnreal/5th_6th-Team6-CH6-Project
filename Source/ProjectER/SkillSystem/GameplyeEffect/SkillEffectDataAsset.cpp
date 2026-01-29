// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplyeEffect/SkillEffectDataAsset.h"
#include "AbilitySystemComponent.h"
#include "SkillSystem/GameAbility/SkillBase.h"


TArray<FGameplayEffectSpecHandle> USkillEffectDataAsset::MakeSpecs(UAbilitySystemComponent* InstigatorASC, USkillBase* InstigatorSkill, AActor* InEffectCauser)
{
	TArray<FGameplayEffectSpecHandle> OutSpecs;

	if (!IsValid(InstigatorASC) || !IsValid(InstigatorSkill)) {
		return OutSpecs;
	}

	AActor* AvatarActor = InstigatorASC->GetAvatarActor();
	AActor* FinalCauser = (InEffectCauser != nullptr) ? InEffectCauser : InstigatorASC->GetAvatarActor();

	int32 Level = InstigatorSkill->GetAbilityLevel();

	for (const FSkillEffectDefinition& SkillEffectDefinition : Data.SkillEffectDefinition)
	{
		if (SkillEffectDefinition.SkillEffectClass)
		{
			FGameplayEffectContextHandle ContextHandle = InstigatorASC->MakeEffectContext();

			// 데이터 설정
			ContextHandle.AddSourceObject(this);
			ContextHandle.AddInstigator(AvatarActor, FinalCauser);

			// Spec 생성
			FGameplayEffectSpecHandle SpecHandle = InstigatorASC->MakeOutgoingSpec(SkillEffectDefinition.SkillEffectClass, Level, ContextHandle);

			if (SpecHandle.IsValid())
			{
				OutSpecs.Add(SpecHandle);
			}
		}
	}

	return OutSpecs;
}