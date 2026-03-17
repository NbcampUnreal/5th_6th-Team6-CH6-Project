// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplayEffectComponent/SummonRangeAtBone.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayEffect.h"
#include "DrawDebugHelpers.h"
#include "Components/SkeletalMeshComponent.h"

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

FTransform USummonRangeAtBone::CalculateSpawnTransform(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const AActor* TargetActor) const
{
	const USummonRangeByBoneGECConfig* const BoneConfig = ResolveTypedConfigFromSpec<USummonRangeByBoneGECConfig>(GESpec);
	if (!IsValid(BoneConfig) || !IsValid(Instigator))
	{
		return FTransform::Identity;
	}

	UWorld* const World = Instigator->GetWorld();
	if (!IsValid(World))
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

	FVector TargetLocation = BaseLocation;
	ApplyCommonSpawnOptions(TargetLocation, CombinedRotation, BoneConfig, Instigator);

	//DrawDebugBox(World, TargetLocation, BoneConfig->CollisionRadius, CombinedRotation.Quaternion(), FColor::Red, false, 5.0f, 0, 2.0f);
	return FTransform(CombinedRotation, TargetLocation);
}