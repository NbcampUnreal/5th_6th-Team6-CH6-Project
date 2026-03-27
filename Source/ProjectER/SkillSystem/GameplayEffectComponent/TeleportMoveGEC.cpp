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
	if (!IsValid(TeleportConfig))
	{
		return;
	}

	UWorld* const World = Instigator->GetWorld();
	const FVector StartLoc = Instigator->GetActorLocation();

	// ── 1. 1차 목적지 계산 (bSweep이면 벽 앞 정지, 아니면 벽 통과) ──
	FVector Destination = CalculateDestination(GESpec, Instigator, Direction, TeleportConfig);

	// ── 2. 네비게이션 메쉬 투사 (강제) ──
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

		// ── 2.5 고립된 네브메쉬 감지 (장애물 내부 등) ──
		// 시작점에서 도착점까지 네비 경로가 존재하는지 확인
		// 빈 메쉬 내부의 네브메쉬는 외부와 연결 안 됨 → 경로 탐색 실패 → 차단
		UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(World, StartLoc, Destination);
		if (!NavPath || !NavPath->IsValid() || NavPath->IsPartial())
		{
			Destination = StartLoc;
		}
	}

	// ── 3. 장애물 끼임 방지 (항상 수행) ──
	// FindTeleportSpot: 액터 충돌 형태 기준으로 겹치지 않는 가장 가까운 안전 위치를 자동 탐색
	if (IsValid(World))
	{
		FRotator InstigatorRot = Instigator->GetActorRotation();
		if (!World->FindTeleportSpot(Instigator, Destination, InstigatorRot))
		{
			// 안전한 위치를 찾지 못하면 시작 지점 유지
			Destination = StartLoc;
		}
	}

	// ── 4. bSweep용 벽 충돌 감지 (벽꿍 효과 전용) ──
	FHitResult HitResult;
	bool bHitWall = false;
	if (TeleportConfig->bSweep && IsValid(World))
	{
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
	}

	// ── 5. 최종 이동 ──

	Instigator->SetActorLocation(Destination, false, nullptr, ETeleportType::TeleportPhysics);
	UpdateLevelTracker(Instigator);

	// 도착 지점 큐 실행 및 Moving 루핑 종료
	ExecuteMoveCue(TeleportConfig->EndVfx, GESpec, Instigator, Destination);
	ExecuteMoveSound(TeleportConfig->EndSound, GESpec, Instigator, Destination);
	RemoveMovingCue(TeleportConfig->MovingVfx, Instigator);
	RemoveMovingSoundCue(TeleportConfig->MovingSound, Instigator);

	if (bHitWall && TeleportConfig->bDetectWallHit)
	{
		HandleWallHit(Instigator, HitResult, TeleportConfig, GESpec);
	}
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
