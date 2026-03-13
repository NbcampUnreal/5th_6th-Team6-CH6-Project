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

FTransform USummonRangeGEC::CalculateSpawnTransform(const FGameplayEffectSpec& GESpec, const AActor* Instigator) const
{
	const USummonRangeByWorldOriginGECConfig* const WorldConfig = ResolveTypedConfigFromSpec<USummonRangeByWorldOriginGECConfig>(GESpec);
	if (!IsValid(WorldConfig))
	{
		return FTransform::Identity;
	}

	const FGameplayEffectContextHandle& EffectContext = GESpec.GetEffectContext();
	const FGameplayEffectContext* const ContextData = EffectContext.Get();
	if (ContextData == nullptr) return FTransform::Identity;

	UWorld* const World = IsValid(Instigator) ? Instigator->GetWorld() : nullptr;
	if (!IsValid(World)) return FTransform::Identity;

	FVector TargetLocation = GetAnyLocation(EffectContext);
	FRotator CombinedRotation = FRotator::ZeroRotator;

	if (WorldConfig->bSpawnZeroRotation)
	{
		CombinedRotation = FRotator::ZeroRotator;
	}
	else if (WorldConfig->bLookAtTargetLocation && IsValid(Instigator))
	{
		const FVector InstigatorLocation = Instigator->GetActorLocation();
		CombinedRotation = FRotationMatrix::MakeFromX(TargetLocation - InstigatorLocation).Rotator();
		CombinedRotation.Pitch = 0.0f;
		CombinedRotation.Roll = 0.0f;
	}
	else
	{
		CombinedRotation = WorldConfig->RotationOffset;
	}

	TargetLocation += CombinedRotation.RotateVector(WorldConfig->LocationOffset);

	if (WorldConfig->bSnapToGround)
	{
		FHitResult FloorHit;
		FVector TraceEnd = TargetLocation;
		TraceEnd.Z -= 1000.0f;

		FCollisionQueryParams QueryParams;
		if (IsValid(Instigator))
		{
			QueryParams.AddIgnoredActor(Instigator);
		}

		if (World->LineTraceSingleByChannel(FloorHit, TargetLocation, TraceEnd, WorldConfig->GroundTraceChannel, QueryParams))
		{
			TargetLocation.X = FloorHit.Location.X;
			TargetLocation.Y = FloorHit.Location.Y;

			float FinalZOffset = WorldConfig->FloatingHeight;
			if (WorldConfig->bUseBoxExtentOffset)
			{
				FinalZOffset += WorldConfig->CollisionRadius.Z;
			}

			TargetLocation.Z = FloorHit.Location.Z + FinalZOffset;
		}
	}

	DrawDebugBox(World, TargetLocation, WorldConfig->CollisionRadius, CombinedRotation.Quaternion(), FColor::Green, false, 5.0f, 0, 2.0f);
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
