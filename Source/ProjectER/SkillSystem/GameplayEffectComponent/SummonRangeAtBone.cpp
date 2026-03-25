// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplayEffectComponent/SummonRangeAtBone.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayEffect.h"
#include "DrawDebugHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "SkillSystem/Actor/BaseRangeOverlapEffectActor/BaseRangeOverlapEffectActor.h"

USummonRangeAtBone::USummonRangeAtBone()
{
	ConfigClass = USummonRangeByBoneGECConfig::StaticClass();
}

TSubclassOf<UBaseGECConfig> USummonRangeAtBone::GetRequiredConfigClass() const
{
	return USummonRangeByBoneGECConfig::StaticClass();
}

bool USummonRangeAtBone::ShouldProcessOnInstigator(const AActor* Instigator) const
{
	if (!IsValid(Instigator))
	{
		return false;
	}

	return Instigator->HasAuthority();
}

FTransform USummonRangeAtBone::CalculateOriginTransform(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const AActor* TargetActor) const
{
	const USummonRangeByBoneGECConfig* const BoneConfig = ResolveTypedConfigFromSpec<USummonRangeByBoneGECConfig>(GESpec);
	if (!IsValid(BoneConfig) || !IsValid(Instigator))
	{
		return FTransform::Identity;
	}

	FVector BaseLocation = Instigator->GetActorLocation();
	FRotator BaseRotation = Instigator->GetActorRotation();

	if (const USkeletalMeshComponent* const Mesh = Instigator->FindComponentByClass<USkeletalMeshComponent>())
	{
		if (Mesh->DoesSocketExist(BoneConfig->BoneName))
		{
			BaseLocation = Mesh->GetSocketLocation(BoneConfig->BoneName);
			BaseRotation = Mesh->GetSocketRotation(BoneConfig->BoneName);
		}
	}

	FRotator CombinedRotation = BaseRotation;
	if (BoneConfig->bUseInstigatorRotation)
	{
		CombinedRotation = Instigator->GetActorRotation();
	}

	return FTransform(CombinedRotation, BaseLocation);
}

void USummonRangeAtBone::InitializeRangeActor(ABaseRangeOverlapEffectActor* RangeActor, const USummonRangeBaseConfig* Config, AActor* Instigator, const FGameplayEffectContextHandle& Context, const FGameplayCueParameters& HitTargetVfxCueParameters, const FGameplayCueParameters& HitTargetSoundCueParameters) const
{
	Super::InitializeRangeActor(RangeActor, Config, Instigator, Context, HitTargetVfxCueParameters, HitTargetSoundCueParameters);

	const USummonRangeByBoneGECConfig* const BoneConfig = Cast<USummonRangeByBoneGECConfig>(Config);
	if (!IsValid(BoneConfig) || !BoneConfig->bAttachToBone || !IsValid(Instigator) || !IsValid(RangeActor))
	{
		return;
	}

	if (USkeletalMeshComponent* const Mesh = Instigator->FindComponentByClass<USkeletalMeshComponent>())
	{
		if (Mesh->DoesSocketExist(BoneConfig->BoneName))
		{
			RangeActor->AttachToComponent(Mesh, FAttachmentTransformRules::KeepWorldTransform, BoneConfig->BoneName);
		}
	}
}