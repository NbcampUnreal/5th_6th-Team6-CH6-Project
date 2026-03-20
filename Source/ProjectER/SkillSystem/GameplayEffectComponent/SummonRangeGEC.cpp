// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplayEffectComponent/SummonRangeGEC.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayEffect.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"

USummonRangeGEC::USummonRangeGEC()
{
	ConfigClass = USummonRangeByWorldOriginGECConfig::StaticClass();
}

TSubclassOf<UBaseGECConfig> USummonRangeGEC::GetRequiredConfigClass() const
{
	return USummonRangeByWorldOriginGECConfig::StaticClass();
}

FTransform USummonRangeGEC::CalculateOriginTransform(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const AActor* TargetActor) const
{
	const USummonRangeByWorldOriginGECConfig* const WorldConfig = ResolveTypedConfigFromSpec<USummonRangeByWorldOriginGECConfig>(GESpec);
	if (!IsValid(WorldConfig))
	{
		return FTransform::Identity;
	}

	const FGameplayEffectContextHandle& EffectContext = GESpec.GetEffectContext();
	const FGameplayEffectContext* const ContextData = EffectContext.Get();
	if (ContextData == nullptr) return FTransform::Identity;

	FVector TargetLocation = GetAnyLocation(EffectContext);
	FRotator CombinedRotation = FRotator::ZeroRotator;

	if (WorldConfig->bLookAtTargetLocation && IsValid(Instigator))
	{
		const FVector InstigatorLocation = Instigator->GetActorLocation();
		CombinedRotation = FRotationMatrix::MakeFromX(TargetLocation - InstigatorLocation).Rotator();
		CombinedRotation.Pitch = 0.0f;
		CombinedRotation.Roll = 0.0f;
	}

	return FTransform(CombinedRotation, TargetLocation);
}

FVector USummonRangeGEC::GetAnyLocation(const FGameplayEffectContextHandle& ContextHandle) const
{
	const FGameplayEffectContext* Context = ContextHandle.Get();
	if (!Context) return FVector::ZeroVector;

	if (Context->HasOrigin()) return Context->GetOrigin();

	if (const FHitResult* Hit = Context->GetHitResult())
	{
		if (!Hit->Location.IsZero()) return Hit->Location;
		if (Hit->GetActor()) return Hit->GetActor()->GetActorLocation();
	}

	return FVector::ZeroVector;
}
