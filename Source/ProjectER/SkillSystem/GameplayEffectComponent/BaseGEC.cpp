#include "SkillSystem/GameplayEffectComponent/BaseGEC.h"
#include "SkillSystem/GameplayEffectComponent/BaseGECConfig.h"
#include "SkillSystem/GameplyeEffect/SkillEffectDataAsset.h"
#include "SkillSystem/GameplayEffectComponent/AdditionalEffectGEC.h"
#include "SkillSystem/GameAbility/SkillBase.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

UBaseGEC::UBaseGEC()
{
	ConfigClass = UBaseGECConfig::StaticClass();
}

TSubclassOf<UBaseGECConfig> UBaseGEC::GetRequiredConfigClass() const
{
	return UBaseGECConfig::StaticClass();
}

void UBaseGEC::OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
	Super::OnGameplayEffectExecuted(ActiveGEContainer, GESpec, PredictionKey);
}

const UBaseGECConfig* UBaseGEC::ResolveBaseConfigFromSpec(const FGameplayEffectSpec& GESpec)
{
	const USkillEffectDataAsset* const SkillDataAsset = Cast<USkillEffectDataAsset>(GESpec.GetEffectContext().GetSourceObject());
	if (!IsValid(SkillDataAsset))
	{
		UE_LOG(LogTemp, Warning, TEXT("UBaseGEC::ResolveBaseConfigFromSpec = !IsValid(SkillDataAsset)"));
		return nullptr;
	}

	const FGameplayTag IndexTag = SkillDataAsset->GetIndexTag();
	const int32 DataIndex = FMath::RoundToInt(GESpec.GetSetByCallerMagnitude(IndexTag, false, -1.0f));
	const FSkillEffectContainer& SkillContainer = SkillDataAsset->GetData();
	if (!SkillContainer.SkillEffectDefinition.IsValidIndex(DataIndex))
	{
        UE_LOG(LogTemp, Warning, TEXT("UBaseGEC::ResolveBaseConfigFromSpec = !SkillContainer.SkillEffectDefinition.IsValidIndex(DataIndex)"));
		return nullptr;
	}

	return SkillContainer.SkillEffectDefinition[DataIndex].Config;
}

void UBaseGEC::GetSkillProcEffects(UAbilitySystemComponent* InstigatorASC, UGameplayAbility* InstigatorSkill, AActor* InEffectCauser, const FGameplayEffectContextHandle& CurrentContext, TArray<FGameplayEffectSpecHandle>& OutSpecs, bool bDefaultConsume)
{
	if (!IsValid(InstigatorASC) || !IsValid(InstigatorSkill))
	{
		return;
	}

	// 1. 버프 태그
	static const FGameplayTag SkillProcTag = FGameplayTag::RequestGameplayTag(FName("Skill.Data.Augments"));

	// 2. 버프 검색
	FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(SkillProcTag));
	TArray<FActiveGameplayEffectHandle> FoundHandles = InstigatorASC->GetActiveEffects(Query);

	if (FoundHandles.Num() > 0)
	{
		const FActiveGameplayEffectHandle& Handle = FoundHandles[0];
		const FActiveGameplayEffect* ActiveGE = InstigatorASC->GetActiveGameplayEffect(Handle);
		
		if (ActiveGE)
		{
			bool bShouldConsume = bDefaultConsume;

			// 3. AdditionalEffectConfig 추출
			const UAdditionalEffectConfig* const ExtraConfig = ResolveTypedConfigFromSpec<UAdditionalEffectConfig>(ActiveGE->Spec);
			if (IsValid(ExtraConfig))
			{
				// 4. 추가 효과들로부터 스펙 생성
				for (const USkillEffectDataAsset* const EffectAsset : ExtraConfig->Bonus)
				{
					if (IsValid(EffectAsset))
					{
						OutSpecs.Append(EffectAsset->MakeSpecs(InstigatorASC, InstigatorSkill, InEffectCauser, CurrentContext));
					}
				}

				// 5. 서버 설정(Config)에서 소모 여부 결정
				bShouldConsume = ExtraConfig->bConsumeBuff;
			}

			// 6. 버프 소모 처리
			if (bShouldConsume)
			{
				InstigatorASC->RemoveActiveGameplayEffect(Handle);
			}
		}
	}
}
