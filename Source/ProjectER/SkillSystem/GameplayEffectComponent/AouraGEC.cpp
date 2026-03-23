// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/GameplayEffectComponent/AouraGEC.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "SkillSystem/Actor/BaseRangeOverlapEffectActor/BaseRangeOverlapEffectActor.h"
#include "SkillSystem/Component/AreaPeriodicEffectComponent.h"
#include "SkillSystem/GameAbility/SkillBase.h"
#include "SkillSystem/GameplyeEffect/SkillEffectDataAsset.h"

UAouraGECConfig::UAouraGECConfig()
{
	// 오라 형태이므로 기본적으로 여러 번 타격 가능해야 함
	bHitOncePerTarget = false;
	LifeSpan = 5.0f; // 기본 지속시간
}

UAouraGEC::UAouraGEC()
{
	ConfigClass = UAouraGECConfig::StaticClass();
}

TSubclassOf<UBaseGECConfig> UAouraGEC::GetRequiredConfigClass() const
{
	return UAouraGECConfig::StaticClass();
}

void UAouraGEC::InitializeRangeActor(ABaseRangeOverlapEffectActor* RangeActor, const USummonRangeBaseConfig* Config, AActor* Instigator, const FGameplayEffectContextHandle& Context, const FGameplayCueParameters& HitTargetCueParameters) const
{
	Super::InitializeRangeActor(RangeActor, Config, Instigator, Context, HitTargetCueParameters);
	if (!IsValid(RangeActor) || !IsValid(Config) || !IsValid(Instigator))
	{
		return;
	}

	const UAouraGECConfig* const AouraConfig = Cast<UAouraGECConfig>(Config);
	if (!IsValid(AouraConfig))
	{
		return;
	}

	// 2. 캐릭터 본에 부착
	UAbilitySystemComponent* const CauserASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Instigator);
	if (const USkeletalMeshComponent* const Mesh = Instigator->FindComponentByClass<USkeletalMeshComponent>())
	{
		if (Mesh->DoesSocketExist(AouraConfig->BoneName))
		{
			// 소켓이 존재하면 해당 본에 부착하여 함께 이동하도록 함
			RangeActor->AttachToComponent(const_cast<USkeletalMeshComponent*>(Mesh), FAttachmentTransformRules::SnapToTargetNotIncludingScale, AouraConfig->BoneName);

			// 부착 후 SnapToTarget에 의해 초기화된 상대 트랜스폼에 Config의 오프셋을 재적용
			RangeActor->SetActorRelativeLocation(AouraConfig->LocationOffset);
			RangeActor->SetActorRelativeRotation(AouraConfig->RotationOffset);
		}
	}

	// 3. 주기적 효과 컴포넌트(AreaPeriodicEffectComponent) 생성 및 설정
	UAreaPeriodicEffectComponent* PeriodicComp = NewObject<UAreaPeriodicEffectComponent>(RangeActor, UAreaPeriodicEffectComponent::StaticClass(), TEXT("AuraPeriodicComponent"));
	if (IsValid(PeriodicComp))
	{
		PeriodicComp->CreationMethod = EComponentCreationMethod::Instance;
		PeriodicComp->SetIsReplicated(true);
		
		// 컴포넌트 등록 및 액터 할당
		PeriodicComp->RegisterComponent();
		RangeActor->AddInstanceComponent(PeriodicComp);
		RangeActor->SetAreaPeriodicComponent(PeriodicComp);

		// 주기 및 즉시 적용 여부 설정
		PeriodicComp->SetupPeriodicEffect(AouraConfig->Period, AouraConfig->bApplyImmediately);
	}
}
