// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/GameplayEffectComponent/ConstantForceMoveGEC.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/CapsuleComponent.h"
#include "GameplayEffect.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "SkillSystem/SkillNiagaraSpawnConfig.h"

UConstantForceMoveGEC::UConstantForceMoveGEC()
{
	ConfigClass = UConstantForceMoveGECConfig::StaticClass();
}

TSubclassOf<UBaseGECConfig> UConstantForceMoveGEC::GetRequiredConfigClass() const
{
	return UConstantForceMoveGECConfig::StaticClass();
}

float UConstantForceMoveGEC::CalculateMoveDuration(const AActor* Instigator, const FVector& Direction, const UMoveBaseConfig* Config) const
{
	const UConstantForceMoveGECConfig* const CFConfig = Cast<UConstantForceMoveGECConfig>(Config);
	if (IsValid(CFConfig) && CFConfig->MoveSpeed > 0.0f)
	{
		return CFConfig->MoveDistance / CFConfig->MoveSpeed;
	}
	return 0.2f;
}

void UConstantForceMoveGEC::Execute(AActor* Instigator, const FVector& Direction, const UMoveBaseConfig* Config, const FGameplayEffectSpec& GESpec) const
{
	const UConstantForceMoveGECConfig* const CFConfig = Cast<UConstantForceMoveGECConfig>(Config);
	ACharacter* const Character = Cast<ACharacter>(Instigator);
	if (!IsValid(CFConfig) || !IsValid(Character))
	{
		return;
	}

	UCharacterMovementComponent* const CMC = Character->GetCharacterMovement();
	if (!IsValid(CMC))
	{
		return;
	}

	const float Duration = (CFConfig->MoveSpeed > 0.0f)
		? (CFConfig->MoveDistance / CFConfig->MoveSpeed)
		: 0.2f;

	if (Character->HasAuthority())
	{
		TSharedPtr<FRootMotionSource_ConstantForce> ConstantForce = MakeShared<FRootMotionSource_ConstantForce>();
		ConstantForce->InstanceName = FName(TEXT("ConstantForceMoveGEC"));
		ConstantForce->AccumulateMode = ERootMotionAccumulateMode::Override;
		ConstantForce->Priority = 5;
		ConstantForce->Force = Direction * CFConfig->MoveSpeed;
		ConstantForce->Duration = Duration;
		ConstantForce->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity;

		CMC->ApplyRootMotionSource(ConstantForce);

		if (CFConfig->bIgnoreUnitCollision)
		{
			SetPawnCollisionIgnore(Character, true);
		}
	}

	const FVector StartLoc = Instigator->GetActorLocation();
	const FVector ExpectedEndLoc = StartLoc + Direction * CFConfig->MoveDistance;

	TWeakObjectPtr<UConstantForceMoveGEC const> WeakThis = this;
	TWeakObjectPtr<AActor> WeakInstigator = Instigator;

	FTimerHandle PostMoveTimer;
	Instigator->GetWorld()->GetTimerManager().SetTimer(
		PostMoveTimer,
		[WeakThis, WeakInstigator, StartLoc, ExpectedEndLoc, ConfigRef = CFConfig, GESpecCopy = GESpec]()
		{
			if (!WeakThis.IsValid() || !WeakInstigator.IsValid() || !IsValid(ConfigRef))
			{
				return;
			}

			if (WeakInstigator->HasAuthority())
			{
				if (ConfigRef->bDetectWallHit)
				{
					const FVector ActualEndLoc = WeakInstigator->GetActorLocation();
					const float ExpectedDist = FVector::Dist(StartLoc, ExpectedEndLoc);
					const float ActualDist = FVector::Dist(StartLoc, ActualEndLoc);

					if (ExpectedDist > 0.0f && ActualDist < ExpectedDist * 0.85f)
					{
						FHitResult FakeHit;
						FakeHit.Location = ActualEndLoc;
						WeakThis->HandleWallHit(WeakInstigator.Get(), FakeHit, ConfigRef, GESpecCopy);
					}
				}

				if (ConfigRef->bIgnoreUnitCollision)
				{
					if (ACharacter* CharPtr = Cast<ACharacter>(WeakInstigator.Get()))
					{
						WeakThis->SetPawnCollisionIgnore(CharPtr, false);
					}
				}
			}

			// 도착 지점 큐 실행 및 Moving 루핑 종료 (서버/클라이언트 공통)
			WeakThis->ExecuteMoveCue(ConfigRef->EndVfx, GESpecCopy, WeakInstigator.Get(), WeakInstigator->GetActorLocation());
			WeakThis->RemoveMovingCue(ConfigRef->MovingVfx, WeakInstigator.Get());

			// SnapToGround 불필요 — CMC가 RootMotionSource 종료 후 자동으로 지면 추적 처리
		},
		Duration + 0.05f,
		false);
}
