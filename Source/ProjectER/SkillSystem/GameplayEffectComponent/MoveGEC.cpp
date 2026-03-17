// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/GameplayEffectComponent/MoveGEC.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameplayEffect.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "SkillSystem/GameAbility/SkillBase.h"
#include "SkillSystem/GameplyeEffect/SkillEffectDataAsset.h"

UMoveGEC::UMoveGEC()
{
	ConfigClass = UMoveGECConfig::StaticClass();
}

TSubclassOf<UBaseGECConfig> UMoveGEC::GetRequiredConfigClass() const
{
	return UMoveGECConfig::StaticClass();
}

void UMoveGEC::OnGameplayEffectApplied(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
	Super::OnGameplayEffectApplied(ActiveGEContainer, GESpec, PredictionKey);

	const FGameplayEffectContextHandle& ContextHandle = GESpec.GetEffectContext();
	const FGameplayEffectContext* const EffectContext = ContextHandle.Get();
	if (EffectContext == nullptr)
	{
		return;
	}

	AActor* const Instigator = IsValid(ContextHandle.GetInstigator())
		? ContextHandle.GetInstigator()
		: ContextHandle.GetEffectCauser();
	if (!IsValid(Instigator))
	{
		return;
	}

	const UMoveGECConfig* const Config = Cast<UMoveGECConfig>(ResolveBaseConfigFromSpec(GESpec));
	if (!IsValid(Config))
	{
		return;
	}

	// 루트 모션 애니메이션 재생 중이면 GEC 이동 무시
	if (Config->bIgnoreIfRootMotion && IsRootMotionActive(Instigator))
	{
		return;
	}

	const FVector StartLocation = Instigator->GetActorLocation();
	const FVector Direction = CalculateMoveDirection(GESpec, Instigator, Config);

	switch (Config->MoveType)
	{
	case EMoveType::Teleport:
	{
		FVector Destination = CalculateDestination(Instigator, Direction, Config);
		ExecuteTeleport(Instigator, Destination, Config);
		break;
	}
	case EMoveType::Launch:
		ExecuteLaunch(Instigator, Direction, Config);
		break;

	case EMoveType::ConstantForce:
		ExecuteConstantForce(Instigator, Direction, Config, GESpec);
		break;
	}

	const FVector EndLocation = Instigator->GetActorLocation();
	ExecuteMoveCues(GESpec, Instigator, StartLocation, EndLocation, Config);
}

bool UMoveGEC::IsRootMotionActive(const AActor* Actor) const
{
	const ACharacter* const Character = Cast<ACharacter>(Actor);
	if (!IsValid(Character))
	{
		return false;
	}

	const UCharacterMovementComponent* const CMC = Character->GetCharacterMovement();
	if (!IsValid(CMC))
	{
		return false;
	}

	// 애니메이션 루트 모션 또는 코드 기반 RootMotionSource 모두 확인
	return CMC->HasAnimRootMotion() || CMC->CurrentRootMotion.HasActiveRootMotionSources();
}

FVector UMoveGEC::CalculateMoveDirection(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const UMoveGECConfig* Config) const
{
	if (!IsValid(Instigator) || !IsValid(Config))
	{
		return FVector::ForwardVector;
	}

	switch (Config->DirectionSource)
	{
	case EMoveDirectionSource::Forward:
		return Instigator->GetActorForwardVector();

	case EMoveDirectionSource::TowardContext:
	{
		const FGameplayEffectContextHandle& Context = GESpec.GetEffectContext();
		if (Context.HasOrigin())
		{
			const FVector ToTarget = (Context.GetOrigin() - Instigator->GetActorLocation());
			if (!ToTarget.IsNearlyZero())
			{
				return ToTarget.GetSafeNormal();
			}
		}
		// Fallback: 전방 사용
		return Instigator->GetActorForwardVector();
	}

	case EMoveDirectionSource::TowardTarget:
	{
		const FGameplayEffectContextHandle& Context = GESpec.GetEffectContext();
		if (const FHitResult* Hit = Context.GetHitResult())
		{
			FVector TargetLocation = FVector::ZeroVector;
			if (!Hit->Location.IsZero())
			{
				TargetLocation = Hit->Location;
			}
			else if (Hit->GetActor())
			{
				TargetLocation = Hit->GetActor()->GetActorLocation();
			}

			const FVector ToTarget = (TargetLocation - Instigator->GetActorLocation());
			if (!ToTarget.IsNearlyZero())
			{
				return ToTarget.GetSafeNormal();
			}
		}
		// Fallback: 전방 사용
		return Instigator->GetActorForwardVector();
	}
	}

	return Instigator->GetActorForwardVector();
}

FVector UMoveGEC::CalculateDestination(const AActor* Instigator, const FVector& Direction, const UMoveGECConfig* Config) const
{
	if (!IsValid(Instigator) || !IsValid(Config))
	{
		return IsValid(Instigator) ? Instigator->GetActorLocation() : FVector::ZeroVector;
	}

	const FVector StartLocation = Instigator->GetActorLocation();
	const FVector TargetLocation = StartLocation + Direction * Config->MoveDistance;

	// Sweep으로 벽 충돌 체크
	if (Config->bSweep)
	{
		UWorld* const World = Instigator->GetWorld();
		if (!IsValid(World))
		{
			return TargetLocation;
		}

		FHitResult HitResult;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(Instigator);

		// Pawn 캡슐 반경을 기준으로 Sweep
		FCollisionShape CapsuleShape = FCollisionShape::MakeSphere(40.0f);
		if (const ACharacter* const Character = Cast<ACharacter>(Instigator))
		{
			const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
			if (IsValid(Capsule))
			{
				CapsuleShape = FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight());
			}
		}

		if (World->SweepSingleByChannel(HitResult, StartLocation, TargetLocation, FQuat::Identity, ECC_WorldStatic, CapsuleShape, QueryParams))
		{
			// 벽에 막힌 위치까지만 이동 (약간 안쪽으로)
			return HitResult.Location;
		}
	}

	return TargetLocation;
}

void UMoveGEC::ExecuteTeleport(AActor* Instigator, const FVector& Destination, const UMoveGECConfig* Config) const
{
	if (!IsValid(Instigator) || !IsValid(Config))
	{
		return;
	}

	FVector FinalDestination = Destination;
	SnapToGround(FinalDestination, Config, Instigator);
	Instigator->SetActorLocation(FinalDestination, false, nullptr, ETeleportType::TeleportPhysics);
}

void UMoveGEC::ExecuteLaunch(AActor* Instigator, const FVector& Direction, const UMoveGECConfig* Config) const
{
	ACharacter* const Character = Cast<ACharacter>(Instigator);
	if (!IsValid(Character) || !IsValid(Config))
	{
		return;
	}

	// 유닛 충돌 무시
	if (Config->bIgnoreUnitCollision)
	{
		if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
		{
			Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

			// 발사는 즉시 힘을 가하는 것이므로, 예상 이동 시간만큼 뒤에 복구
			const float Duration = (Config->MoveSpeed > 0.0f) ? (Config->MoveDistance / Config->MoveSpeed) : 0.5f;
			FTimerHandle RestoreTimer;
			TWeakObjectPtr<ACharacter> WeakChar = Character;
			Character->GetWorld()->GetTimerManager().SetTimer(RestoreTimer, [WeakChar]()
			{
				if (WeakChar.IsValid())
				{
					if (UCapsuleComponent* Comp = WeakChar->GetCapsuleComponent())
					{
						Comp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
					}
				}
			}, Duration, false);
		}
	}

	Character->LaunchCharacter(Direction * Config->MoveSpeed, Config->bXYOverride, Config->bZOverride);
}

void UMoveGEC::ExecuteConstantForce(AActor* Instigator, const FVector& Direction, const UMoveGECConfig* Config, const FGameplayEffectSpec& GESpec) const
{
	ACharacter* const Character = Cast<ACharacter>(Instigator);
	if (!IsValid(Character) || !IsValid(Config))
	{
		return;
	}

	UCharacterMovementComponent* const CMC = Character->GetCharacterMovement();
	if (!IsValid(CMC))
	{
		return;
	}

	// Duration = Distance / Speed
	const float Duration = (Config->MoveSpeed > 0.0f)
		? (Config->MoveDistance / Config->MoveSpeed)
		: 0.2f;

	TSharedPtr<FRootMotionSource_ConstantForce> ConstantForce = MakeShared<FRootMotionSource_ConstantForce>();
	ConstantForce->InstanceName = FName(TEXT("MoveGEC_ConstantForce"));
	ConstantForce->AccumulateMode = ERootMotionAccumulateMode::Override;
	ConstantForce->Priority = 5;
	ConstantForce->Force = Direction * Config->MoveSpeed;
	ConstantForce->Duration = Duration;
	ConstantForce->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	ConstantForce->FinishVelocityParams.SetVelocity = FVector::ZeroVector;

	// 유닛 충돌 무시
	if (Config->bIgnoreUnitCollision)
	{
		if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
		{
			Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		}
	}

	const uint16 RootMotionSourceID = CMC->ApplyRootMotionSource(ConstantForce);

	// 벽꿍 감지: ConstantForce 이동 완료 후 타이머로 위치 체크
	if (Config->bDetectWallHit && IsValid(Instigator->GetWorld()))
	{
		// 이동 실제 거리 vs 예상 거리 비교하여 벽 감지
		const FVector StartLoc = Instigator->GetActorLocation();

		FTimerHandle WallCheckTimer;
		TWeakObjectPtr<AActor> WeakInstigator = Instigator;
		const UMoveGECConfig* WeakConfig = Config;
		const FVector ExpectedEndLoc = StartLoc + Direction * Config->MoveDistance;

		Instigator->GetWorld()->GetTimerManager().SetTimer(
			WallCheckTimer,
			[WeakInstigator, WeakConfig, ExpectedEndLoc, StartLoc, GESpecCopy = GESpec, this]()
			{
				if (!WeakInstigator.IsValid() || !IsValid(WeakConfig))
				{
					return;
				}

				const FVector ActualEndLoc = WeakInstigator->GetActorLocation();
				const float ExpectedDist = FVector::Dist(StartLoc, ExpectedEndLoc);
				const float ActualDist = FVector::Dist(StartLoc, ActualEndLoc);

				// 실제 이동 거리가 기대치보다 유의미하게 짧으면 벽에 막힌 것으로 판단
				if (ExpectedDist > 0.0f && ActualDist < ExpectedDist * 0.85f)
				{
					FHitResult FakeHit;
					FakeHit.Location = ActualEndLoc;
					HandleWallHit(WeakInstigator.Get(), FakeHit, WeakConfig, GESpecCopy);
				}

				// 유닛 충돌 복구
				if (WeakConfig->bIgnoreUnitCollision)
				{
					if (ACharacter* CharacterPtr = Cast<ACharacter>(WeakInstigator.Get()))
					{
						if (UCapsuleComponent* Capsule = CharacterPtr->GetCapsuleComponent())
						{
							Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
						}
					}
				}

				// 지면 보정
				FVector FinalLoc = ActualEndLoc;
				SnapToGround(FinalLoc, WeakConfig, WeakInstigator.Get());
				if (!FinalLoc.Equals(ActualEndLoc, 1.0f))
				{
					WeakInstigator->SetActorLocation(FinalLoc, false, nullptr, ETeleportType::TeleportPhysics);
				}
			},
			Duration + 0.05f,
			false);
	}
}

void UMoveGEC::HandleWallHit(AActor* Instigator, const FHitResult& Hit, const UMoveGECConfig* Config, const FGameplayEffectSpec& GESpec) const
{
	if (!IsValid(Instigator) || !IsValid(Config))
	{
		return;
	}

	UAbilitySystemComponent* const InstigatorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Instigator);
	if (!IsValid(InstigatorASC))
	{
		return;
	}

	const FGameplayEffectContextHandle& ContextHandle = GESpec.GetEffectContext();
	USkillBase* const Skill = const_cast<USkillBase*>(Cast<USkillBase>(ContextHandle.GetAbility()));

	for (USkillEffectDataAsset* const EffectData : Config->WallHitApplied)
	{
		if (!IsValid(EffectData))
		{
			continue;
		}

		// WallHit 효과는 자기 자신(Instigator)에게 적용 (스턴 등)
		for (FGameplayEffectSpecHandle& Spec : EffectData->MakeSpecs(InstigatorASC, Skill, Instigator, ContextHandle))
		{
			if (!Spec.IsValid())
			{
				continue;
			}

			InstigatorASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), InstigatorASC);
		}
	}
}

void UMoveGEC::SnapToGround(FVector& InOutLocation, const UMoveGECConfig* Config, const AActor* Instigator) const
{
	if (!IsValid(Config) || !IsValid(Instigator))
	{
		return;
	}

	UWorld* const World = Instigator->GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	FHitResult FloorHit;
	FVector TraceStart = InOutLocation;
	FVector TraceEnd = TraceStart;
	TraceEnd.Z -= Config->GroundTraceDistance;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Instigator);

	if (World->LineTraceSingleByChannel(FloorHit, TraceStart, TraceEnd, Config->GroundTraceChannel, QueryParams))
	{
		InOutLocation.Z = FloorHit.Location.Z;
	}
}

void UMoveGEC::ExecuteMoveCues(const FGameplayEffectSpec& GESpec, AActor* Instigator, const FVector& StartLocation, const FVector& EndLocation, const UMoveGECConfig* Config) const
{
	if (!IsValid(Instigator) || !IsValid(Config))
	{
		return;
	}

	UAbilitySystemComponent* const InstigatorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Instigator);
	if (!IsValid(InstigatorASC))
	{
		return;
	}

	const FGameplayEffectContextHandle& ContextHandle = GESpec.GetEffectContext();
	FScopedPredictionWindow ForcedWindow(InstigatorASC, FPredictionKey(), false);

	// Start VFX
	if (Config->StartVfx.CueTag.IsValid())
	{
		FGameplayCueParameters StartCueParams(GESpec);
		StartCueParams.OriginalTag = Config->StartVfx.CueTag;
		StartCueParams.Instigator = ContextHandle.GetInstigator();
		StartCueParams.EffectCauser = Instigator;
		StartCueParams.Location = StartLocation;
		InstigatorASC->ExecuteGameplayCue(Config->StartVfx.CueTag, StartCueParams);
	}

	// Moving VFX (캐릭터에 부착)
	if (Config->MovingVfx.CueTag.IsValid())
	{
		FGameplayCueParameters MovingCueParams(GESpec);
		MovingCueParams.OriginalTag = Config->MovingVfx.CueTag;
		MovingCueParams.Instigator = ContextHandle.GetInstigator();
		MovingCueParams.EffectCauser = Instigator;
		MovingCueParams.Location = StartLocation;
		MovingCueParams.TargetAttachComponent = Instigator->GetRootComponent();
		InstigatorASC->ExecuteGameplayCue(Config->MovingVfx.CueTag, MovingCueParams);
	}

	// End VFX
	if (Config->EndVfx.CueTag.IsValid())
	{
		FGameplayCueParameters EndCueParams(GESpec);
		EndCueParams.OriginalTag = Config->EndVfx.CueTag;
		EndCueParams.Instigator = ContextHandle.GetInstigator();
		EndCueParams.EffectCauser = Instigator;
		EndCueParams.Location = EndLocation;
		InstigatorASC->ExecuteGameplayCue(Config->EndVfx.CueTag, EndCueParams);
	}
}
