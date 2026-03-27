// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/GameplayEffectComponent/TeleportMoveGEC.h"

#include "Components/CapsuleComponent.h"
#include "GameplayEffect.h"
#include "GameFramework/Character.h"
#include "SkillSystem/SkillNiagaraSpawnConfig.h"
#include "LevelManagement/LevelAreaTrackerComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"

UTeleportMoveGEC::UTeleportMoveGEC()
{
	ConfigClass = UTeleportMoveGECConfig::StaticClass();
}

TSubclassOf<UBaseGECConfig> UTeleportMoveGEC::GetRequiredConfigClass() const
{
	return UTeleportMoveGECConfig::StaticClass();
}

float UTeleportMoveGEC::CalculateMoveDuration(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const FVector& Direction, const UMoveBaseConfig* Config) const
{
	return 0.15f;
}

void UTeleportMoveGEC::Execute(AActor* Instigator, const FVector& Direction, const UMoveBaseConfig* Config, const FGameplayEffectSpec& GESpec) const
{
	const UTeleportMoveGECConfig* const TeleportConfig = Cast<UTeleportMoveGECConfig>(Config);
	if (!IsValid(TeleportConfig)) return;

	UWorld* const World = Instigator->GetWorld();
	if (!IsValid(World)) return;

	const FVector StartLoc = Instigator->GetActorLocation();
	FVector Destination = CalculateDestination(GESpec, Instigator, Direction, TeleportConfig);

	// ── 1. 공통 안전성 검증 (네브메쉬 투사 및 경로 도달 가능성 확인) ──
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (NavSys)
	{
		FNavLocation NavLoc;
		if (NavSys->ProjectPointToNavigation(Destination, NavLoc, FVector(TeleportConfig->NavProjectionRadius)))
		{
			Destination = NavLoc.Location;

			// NavMesh는 바닥 표면 높이를 반환하지만, 캐릭터 액터 위치는 캡슐 중심이어야 함
			if (const ACharacter* const Character = Cast<ACharacter>(Instigator))
			{
				if (const UCapsuleComponent* const Capsule = Character->GetCapsuleComponent())
				{
					Destination.Z += Capsule->GetScaledCapsuleHalfHeight();
				}
			}
		}
		else
		{
			Destination = StartLoc;
		}

		// 고립된 네브메쉬 감지 (장애물 내부 등)
		UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(World, StartLoc, Destination);
		if (!NavPath || !NavPath->IsValid() || NavPath->IsPartial())
		{
			Destination = StartLoc;
		}
	}

	// ── 2. 장애물 끼임 방지 (항상 수행) ──
	FRotator InstigatorRot = Instigator->GetActorRotation();
	if (!World->FindTeleportSpot(Instigator, Destination, InstigatorRot))
	{
		Destination = StartLoc;
	}

	// ── 3. 모드별 로직 수행 (bSweep 여부에 따른 처리) ──
	FHitResult HitResult;
	bool bHitWall = false;

	if (TeleportConfig->bSweep)
	{
		// bSweep 모드: 벽 충돌 감지 및 벽꿍 효과 처리
		FCollisionQueryParams WallQueryParams;
		WallQueryParams.AddIgnoredActor(Instigator);

		FCollisionShape WallShape = FCollisionShape::MakeSphere(40.0f);
		if (const ACharacter* const Character = Cast<ACharacter>(Instigator))
		{
			if (const UCapsuleComponent* const Capsule = Character->GetCapsuleComponent())
			{
				WallShape = FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight());
			}
		}

		const FVector FinalDest = StartLoc + Direction * TeleportConfig->MoveDistance;
		bHitWall = World->SweepSingleByChannel(HitResult, StartLoc, FinalDest, FQuat::Identity, ECC_WorldStatic, WallShape, WallQueryParams);

		if (bHitWall && TeleportConfig->bDetectWallHit)
		{
			HandleWallHit(Instigator, HitResult, TeleportConfig, GESpec);
		}
	}

	// ── 4. 최종 이동 및 이펙트 실행 ──
	Instigator->SetActorLocation(Destination, false, nullptr, ETeleportType::TeleportPhysics);
	UpdateLevelTracker(Instigator);

	ExecuteMoveCue(TeleportConfig->EndVfx, GESpec, Instigator, Destination);
	ExecuteMoveSound(TeleportConfig->EndSound, GESpec, Instigator, Destination);
	RemoveMovingCue(TeleportConfig->MovingVfx, Instigator);
	RemoveMovingSoundCue(TeleportConfig->MovingSound, Instigator);
}

FVector UTeleportMoveGEC::CalculateDestination(const FGameplayEffectSpec& GESpec, AActor* Instigator, const FVector& Direction, const UTeleportMoveGECConfig* Config) const
{
	if (!IsValid(Instigator) || !IsValid(Config))
	{
		return IsValid(Instigator) ? Instigator->GetActorLocation() : FVector::ZeroVector;
	}

	const FVector StartLoc = Instigator->GetActorLocation();
	const FVector TargetLoc = CalculateTargetLocation(GESpec, Instigator, Config);

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
		// 벽 끼임 방지를 위해 노멀 방향으로 약간 물러남
		return HitResult.Location + HitResult.Normal * Config->TeleportSafetyOffset;
	}

	return TargetLoc;
}

void UTeleportMoveGEC::UpdateLevelTracker(AActor* Actor) const
{
	if (!IsValid(Actor)) return;
	if (ULevelAreaTrackerComponent* Tracker = Actor->FindComponentByClass<ULevelAreaTrackerComponent>()) Tracker->UpdateArea();
}
