// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplyeEffect/SkillEffectDataAsset.h"
#include "AbilitySystemComponent.h"
#include "SkillSystem/GameAbility/SkillBase.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponent.h"


USkillEffectDataAsset::USkillEffectDataAsset()
{
	IndexTag = FGameplayTag::RequestGameplayTag(FName("Skill.Data.EffectIndex"));
}

TArray<FGameplayEffectSpecHandle> USkillEffectDataAsset::MakeSpecs(UAbilitySystemComponent* InstigatorASC, USkillBase* InstigatorSkill, AActor* InEffectCauser)
{
	TArray<FGameplayEffectSpecHandle> OutSpecs;

	if (!IsValid(InstigatorASC) || !IsValid(InstigatorSkill)) {
		return OutSpecs;
	}

	AActor* AvatarActor = InstigatorASC->GetAvatarActor();
	AActor* FinalCauser = (InEffectCauser != nullptr) ? InEffectCauser : InstigatorASC->GetAvatarActor();

	int32 Level = InstigatorSkill->GetAbilityLevel();

	for (int32 i = 0; i < Data.SkillEffectDefinition.Num(); ++i)
	{
		const FSkillEffectDefinition& Def = Data.SkillEffectDefinition[i];
		if (Def.SkillEffectClass == nullptr) continue;

		FGameplayEffectContextHandle ContextHandle = InstigatorASC->MakeEffectContext();
		ContextHandle.AddSourceObject(this);
		ContextHandle.AddInstigator(AvatarActor, FinalCauser);

		FGameplayEffectSpecHandle SpecHandle = InstigatorASC->MakeOutgoingSpec(Def.SkillEffectClass, Level, ContextHandle);
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(IndexTag, static_cast<float>(i));
			OutSpecs.Add(SpecHandle);
		}
	}

	return OutSpecs;
}