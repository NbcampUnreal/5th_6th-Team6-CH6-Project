// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/GameplayEffectComponent/ConstantForceMoveGEC.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/CapsuleComponent.h"
#include "GameplayEffect.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "SkillSystem/SkillNiagaraSpawnConfig.h"
#include "TimerManager.h"

UConstantForceMoveGEC::UConstantForceMoveGEC()
{
	ConfigClass = UConstantForceMoveGECConfig::StaticClass();
}

TSubclassOf<UBaseGECConfig> UConstantForceMoveGEC::GetRequiredConfigClass() const
{
	return UConstantForceMoveGECConfig::StaticClass();
}

float UConstantForceMoveGEC::CalculateMoveDuration(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const FVector& Direction, const UMoveBaseConfig* Config) const
{
	const UConstantForceMoveGECConfig* const CFConfig = Cast<UConstantForceMoveGECConfig>(Config);
	if (IsValid(CFConfig) && CFConfig->MoveSpeed > 0.0f)
	{
		const FVector TargetLoc = CalculateTargetLocation(GESpec, Instigator, CFConfig);
		const float Distance = FVector::Dist(Instigator->GetActorLocation(), TargetLoc);
		return Distance / CFConfig->MoveSpeed;
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

	const FVector TargetLoc = CalculateTargetLocation(GESpec, Instigator, CFConfig);
	const float Distance = FVector::Dist(Instigator->GetActorLocation(), TargetLoc);
	const float Duration = (CFConfig->MoveSpeed > 0.0f)
		? (Distance / CFConfig->MoveSpeed)
		: 0.2f;

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

	const FVector StartLoc = Instigator->GetActorLocation();
	const FVector ExpectedEndLoc = TargetLoc;

	TWeakObjectPtr<UConstantForceMoveGEC const> WeakThis = this;
	TWeakObjectPtr<AActor> WeakInstigator = Instigator;

	FTimerHandle PostMoveTimer;
	Instigator->GetWorld()->GetTimerManager().SetTimer(
    PostMoveTimer,
    [WeakThis, WeakInstigator, StartLoc, ExpectedEndLoc, ConfigRef = CFConfig, GESpecCopy = GESpec]()
    {
        // 1. 유효성 검사 (가장 먼저 수행)
        if (!WeakThis.IsValid() || !WeakInstigator.IsValid() || !IsValid(ConfigRef))
        {
            return;
        }

        AActor* InstigatorPtr = WeakInstigator.Get();
        const FVector ActualEndLoc = InstigatorPtr->GetActorLocation();

        // 2. 벽 충돌 감지 로직
        if (ConfigRef->bDetectWallHit)
        {
            const float ExpectedDist = FVector::Dist(StartLoc, ExpectedEndLoc);
            const float ActualDist = FVector::Dist(StartLoc, ActualEndLoc);

            // 예상 거리보다 현저히 적게 이동했다면 벽에 부딪힌 것으로 간주
            if (ExpectedDist > 0.0f && ActualDist < ExpectedDist * 0.85f)
            {
                FHitResult FakeHit;
                FakeHit.Location = ActualEndLoc;
                FakeHit.ImpactPoint = ActualEndLoc; // ImpactPoint도 채워주는 것이 안전합니다.
                
                WeakThis->HandleWallHit(InstigatorPtr, FakeHit, ConfigRef, GESpecCopy);
            }
        }

        // 3. 충돌 무시 복구 로직
        if (ConfigRef->bIgnoreUnitCollision)
        {
            if (ACharacter* CharPtr = Cast<ACharacter>(InstigatorPtr))
            {
                WeakThis->SetPawnCollisionIgnore(CharPtr, false);
            }
        }
        WeakThis->ExecuteMoveCue(ConfigRef->EndVfx, GESpecCopy, InstigatorPtr, ActualEndLoc);
        WeakThis->ExecuteMoveSound(ConfigRef->EndSound, GESpecCopy, InstigatorPtr, ActualEndLoc);
        WeakThis->RemoveMovingCue(ConfigRef->MovingVfx, InstigatorPtr);
        WeakThis->RemoveMovingSoundCue(ConfigRef->MovingSound, InstigatorPtr);

    },
    Duration + 0.05f,
    false);
}
