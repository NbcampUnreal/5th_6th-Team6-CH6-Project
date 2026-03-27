// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/GameplayEffectComponent/LaunchMoveGEC.h"

#include "Components/CapsuleComponent.h"
#include "GameplayEffect.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SkillSystem/SkillNiagaraSpawnConfig.h"

ULaunchMoveGEC::ULaunchMoveGEC()
{
	ConfigClass = ULaunchMoveGECConfig::StaticClass();
}

TSubclassOf<UBaseGECConfig> ULaunchMoveGEC::GetRequiredConfigClass() const
{
	return ULaunchMoveGECConfig::StaticClass();
}

float ULaunchMoveGEC::CalculateMoveDuration(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const FVector& Direction, const UMoveBaseConfig* Config) const
{
	const ULaunchMoveGECConfig* const LaunchConfig = Cast<ULaunchMoveGECConfig>(Config);
	const ACharacter* const Character = Cast<ACharacter>(Instigator);
	if (!IsValid(LaunchConfig) || !IsValid(Character))
	{
		return 0.25f;
	}

	const FVector TargetLoc = CalculateTargetLocation(GESpec, Instigator, LaunchConfig);
	const float Distance = FVector::Dist(Instigator->GetActorLocation(), TargetLoc);

	if (LaunchConfig->VerticalLaunchSpeed > 0.0f)
	{
		const UCharacterMovementComponent* const CMC = Character->GetCharacterMovement();
		const float Gravity = IsValid(CMC) ? -CMC->GetGravityZ() : 980.0f;
		if (Gravity > 0.0f)
		{
			// t = 2 * Vz / g
			return (2.0f * LaunchConfig->VerticalLaunchSpeed) / Gravity;
		}
	}

	return 0.25f; // 지면 발사 기본 예상 시간
}

void ULaunchMoveGEC::Execute(AActor* Instigator, const FVector& Direction, const UMoveBaseConfig* Config, const FGameplayEffectSpec& GESpec) const
{
	const ULaunchMoveGECConfig* const LaunchConfig = Cast<ULaunchMoveGECConfig>(Config);
	ACharacter* const Character = Cast<ACharacter>(Instigator);
	if (!IsValid(LaunchConfig) || !IsValid(Character))
	{
		return;
	}

	// 발사 속도 계산
	const FVector TargetLoc = CalculateTargetLocation(GESpec, Instigator, LaunchConfig);
	const float Distance = FVector::Dist(Instigator->GetActorLocation(), TargetLoc);

	float HorizontalSpeed = 0.0f;
	const float VerticalSpeed = LaunchConfig->VerticalLaunchSpeed;

	if (VerticalSpeed > 0.0f)
	{
		// 1. 도약 (Leap): 중력과 수직 속도를 이용해 체공 시간을 구하고, MoveDistance에 낙하하도록 수평 속도 계산
		UCharacterMovementComponent* const CMC = Character->GetCharacterMovement();
		const float Gravity = IsValid(CMC) ? -CMC->GetGravityZ() : 980.0f;

		if (Gravity > 0.0f)
		{
			// t = 2 * Vz / g (올라갔다 내려오는 시간)
			const float TimeInAir = (2.0f * VerticalSpeed) / Gravity;
			HorizontalSpeed = (TimeInAir > 0.05f) ? (Distance / TimeInAir) : 0.0f;
		}
	}
	else
	{
		// 2. 지면 발사: 특정 예상 도달 시간(예: 0.25초)을 기준으로 초기 속도 부여
		const float TargetTime = 0.25f;
		HorizontalSpeed = Distance / TargetTime;
	}

	// 최종 속도 벡터 생성
	const FVector LaunchVelocity = (Direction * HorizontalSpeed) + (FVector::UpVector * VerticalSpeed);

	// 예상 이동 시간 계산 (타이머용)
	const float PredictDuration = (HorizontalSpeed > 0.0f) ? (Distance / HorizontalSpeed) : 0.5f;

	// 유닛 충돌 무시 (예상 이동 시간 동안)
	if (LaunchConfig->bIgnoreUnitCollision)
	{
		SetPawnCollisionIgnore(Character, true);

		TWeakObjectPtr<ULaunchMoveGEC const> WeakThis = this;
		TWeakObjectPtr<ACharacter> WeakChar = Character;
		FTimerHandle RestoreTimer;
		Character->GetWorld()->GetTimerManager().SetTimer(
			RestoreTimer,
			[WeakThis, WeakChar, GESpec, LaunchConfig]()
			{
				if (WeakThis.IsValid() && WeakChar.IsValid())
				{
					if (LaunchConfig)
					{
						// 도착(착지) 큐 실행 및 Moving 루핑 종료
						WeakThis->ExecuteMoveCue(LaunchConfig->EndVfx, GESpec, WeakChar.Get(), WeakChar->GetActorLocation());
						WeakThis->ExecuteMoveSound(LaunchConfig->EndSound, GESpec, WeakChar.Get(), WeakChar->GetActorLocation());
						WeakThis->RemoveMovingCue(LaunchConfig->MovingVfx, WeakChar.Get());
						WeakThis->RemoveMovingSoundCue(LaunchConfig->MovingSound, WeakChar.Get());

						// 충돌 무시는 서버에서만 제어
						if (LaunchConfig->bIgnoreUnitCollision && WeakChar->HasAuthority())
						{
							WeakThis->SetPawnCollisionIgnore(WeakChar.Get(), false);
						}
					}
				}
			},
			PredictDuration,
			false);
	}
	else
	{
		// 충돌 무시는 없지만 EndVfx는 여전히 타이머로 실행해야 함 (비행 후 착지 시점)
		TWeakObjectPtr<ULaunchMoveGEC const> WeakThis = this;
		TWeakObjectPtr<ACharacter> WeakChar = Character;
		FTimerHandle EndVfxTimer;
		Character->GetWorld()->GetTimerManager().SetTimer(
			EndVfxTimer,
			[WeakThis, WeakChar, GESpec, LaunchConfig]()
			{
				if (WeakThis.IsValid() && WeakChar.IsValid() && LaunchConfig)
				{
					WeakThis->ExecuteMoveCue(LaunchConfig->EndVfx, GESpec, WeakChar.Get(), WeakChar->GetActorLocation());
					WeakThis->ExecuteMoveSound(LaunchConfig->EndSound, GESpec, WeakChar.Get(), WeakChar->GetActorLocation());
					WeakThis->RemoveMovingCue(LaunchConfig->MovingVfx, WeakChar.Get());
					WeakThis->RemoveMovingSoundCue(LaunchConfig->MovingSound, WeakChar.Get());
				}
			},
			PredictDuration,
			false);
	}

    // 캐릭터 상태 변경 (확실히 뜨게 함)
    if (VerticalSpeed > 0.0f || LaunchConfig->bZOverride)
    {
        if (UCharacterMovementComponent *CMC = Character->GetCharacterMovement())
        {
            CMC->SetMovementMode(MOVE_Falling);
        }
    }

    Character->LaunchCharacter(LaunchVelocity, LaunchConfig->bXYOverride, LaunchConfig->bZOverride);
}
