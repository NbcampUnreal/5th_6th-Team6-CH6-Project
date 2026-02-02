// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplyeEffect/SkillEffectDataAsset.h"
#include "AbilitySystemComponent.h"
#include "SkillSystem/GameAbility/SkillBase.h"
#include "GameplayEffect.h"


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
			FGameplayEffectSpec* Spec = SpecHandle.Data.Get();
			if (Spec)
			{
				//FGameplayModifierInfo NewModInfo;
				//NewModInfo.Attribute = UMyAttributeSet::GetDefenseAttribute();
				//NewModInfo.ModifierOp = EGameplayModOp::Additive;
				//NewModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-10.f));

				//// Spec의 Modifiers 배열에 강제로 삽입
				//// 단, Modifiers는 FModifierSpec 배열이므로 포맷을 맞춰야 합니다.
				//FModifierSpec NewModSpec(NewModInfo);
				//Spec->Modifiers.Add(NewModSpec);
				//ModifiedAttributes
				/*for (FGameplayEffectModifiedAttribute& ModSpec : Spec->ModifiedAttributes)
				{
					
					ModSpec.Attribute = Data.TargetAttribute;
				}*/
			}


			if (SpecHandle.IsValid())
			{
				OutSpecs.Add(SpecHandle);
			}
		}
	}

	return OutSpecs;
}