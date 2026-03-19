// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/GameplayEffectComponent/TeleportMoveGEC.h"

#include "Components/CapsuleComponent.h"
#include "GameplayEffect.h"
#include "GameFramework/Character.h"
#include "SkillSystem/SkillNiagaraSpawnConfig.h"

UTeleportMoveGEC::UTeleportMoveGEC()
{
	ConfigClass = UTeleportMoveGECConfig::StaticClass();
}

TSubclassOf<UBaseGECConfig> UTeleportMoveGEC::GetRequiredConfigClass() const
{
	return UTeleportMoveGECConfig::StaticClass();
}

float UTeleportMoveGEC::CalculateMoveDuration(const AActor* Instigator, const FVector& Direction, const UMoveBaseConfig* Config) const
{
	return 0.15f;
}

void UTeleportMoveGEC::Execute(AActor* Instigator, const FVector& Direction, const UMoveBaseConfig* Config, const FGameplayEffectSpec& GESpec) const
{
	const UTeleportMoveGECConfig* const TeleportConfig = Cast<UTeleportMoveGECConfig>(Config);
	if (!IsValid(TeleportConfig))
	{
		return;
	}

	FHitResult HitResult;
	FVector Destination = CalculateDestination(Instigator, Direction, TeleportConfig);

	bool bHitWall = false;
	if (TeleportConfig->bSweep)
	{
		UWorld* const World = Instigator->GetWorld();
		if (IsValid(World))
		{
			const FVector StartLoc = Instigator->GetActorLocation();
			const FVector FinalDest = StartLoc + Direction * TeleportConfig->MoveDistance;

			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(Instigator);

			FCollisionShape CapsuleShape = FCollisionShape::MakeSphere(40.0f);
			if (const ACharacter* const Character = Cast<ACharacter>(Instigator))
			{
				if (const UCapsuleComponent* const Capsule = Character->GetCapsuleComponent())
				{
					CapsuleShape = FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight());
				}
			}

			bHitWall = World->SweepSingleByChannel(HitResult, StartLoc, FinalDest, FQuat::Identity, ECC_WorldStatic, CapsuleShape, QueryParams);
		}
	}

	SnapToGround(Destination, TeleportConfig, Instigator);

	Instigator->SetActorLocation(Destination, false, nullptr, ETeleportType::TeleportPhysics);

	// 도착 지점 큐 실행 및 Moving 루핑 종료
	ExecuteMoveCue(TeleportConfig->EndVfx, GESpec, Instigator, Destination);
	RemoveMovingCue(TeleportConfig->MovingVfx, Instigator);

	if (bHitWall && TeleportConfig->bDetectWallHit)
	{
		HandleWallHit(Instigator, HitResult, TeleportConfig, GESpec);
	}
}

FVector UTeleportMoveGEC::CalculateDestination(const AActor* Instigator, const FVector& Direction, const UTeleportMoveGECConfig* Config) const
{
	if (!IsValid(Instigator) || !IsValid(Config))
	{
		return IsValid(Instigator) ? Instigator->GetActorLocation() : FVector::ZeroVector;
	}

	const FVector StartLoc = Instigator->GetActorLocation();
	const FVector TargetLoc = StartLoc + Direction * Config->MoveDistance;

	if (!Config->bSweep)
	{
		return TargetLoc;
	}

	UWorld* const World = Instigator->GetWorld();
	if (!IsValid(World))
	{
		return TargetLoc;
	}

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Instigator);

	FCollisionShape CapsuleShape = FCollisionShape::MakeSphere(40.0f);
	if (const ACharacter* const Character = Cast<ACharacter>(Instigator))
	{
		if (const UCapsuleComponent* const Capsule = Character->GetCapsuleComponent())
		{
			CapsuleShape = FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight());
		}
	}

	if (World->SweepSingleByChannel(HitResult, StartLoc, TargetLoc, FQuat::Identity, ECC_WorldStatic, CapsuleShape, QueryParams))
	{
		return HitResult.Location;
	}

	return TargetLoc;
}
