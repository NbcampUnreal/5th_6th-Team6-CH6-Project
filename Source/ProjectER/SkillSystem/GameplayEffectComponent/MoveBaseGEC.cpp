// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/GameplayEffectComponent/MoveBaseGEC.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameplayEffect.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SkillSystem/GameAbility/SkillBase.h"
#include "SkillSystem/GameplyeEffect/SkillEffectDataAsset.h"
#include "SkillSystem/SkillNiagaraSpawnConfig.h"

UMoveBaseGEC::UMoveBaseGEC()
{
	ConfigClass = UMoveBaseConfig::StaticClass();
}

void UMoveBaseGEC::OnGameplayEffectApplied(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
	Super::OnGameplayEffectApplied(ActiveGEContainer, GESpec, PredictionKey);

	const FGameplayEffectContextHandle& ContextHandle = GESpec.GetEffectContext();
	if (ContextHandle.Get() == nullptr)
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

	const UMoveBaseConfig* const Config = Cast<UMoveBaseConfig>(ResolveBaseConfigFromSpec(GESpec));
	if (!IsValid(Config))
	{
		return;
	}

	// 루트 모션 애니메이션 재생 중이면 이동 무시
	if (Config->bIgnoreIfRootMotion && IsRootMotionActive(Instigator))
	{
		return;
	}

	const FVector StartLoc = Instigator->GetActorLocation();
	const FVector Direction = CalculateMoveDirection(GESpec, Instigator, Config);

	// 시작 큐 실행
	ExecuteMoveCue(Config->StartVfx, GESpec, Instigator, StartLoc);
	// Moving 루핑 큐 (방향과 속도를 전달하여 클라이언트 동기화 지원)
	AddMovingCue(Config->MovingVfx, GESpec, Instigator, Direction, Config->MoveDistance / CalculateMoveDuration(Instigator, Direction, Config));

	// 파생 클래스가 실제 이동 방식 구현 (EndVfx는 파생 클래스 종료 시점에 직접 실행)
	Execute(Instigator, Direction, Config, GESpec);

	// 애니메이션 속도 동기화
	if (ACharacter* Character = Cast<ACharacter>(Instigator))
	{
		const float Duration = CalculateMoveDuration(Instigator, Direction, Config);
		AdjustActiveMontageRate(Character, Duration, Config);
	}
}

bool UMoveBaseGEC::IsRootMotionActive(const AActor* Actor) const
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

	return CMC->HasAnimRootMotion() || CMC->CurrentRootMotion.HasActiveRootMotionSources();
}

FVector UMoveBaseGEC::CalculateMoveDirection(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const UMoveBaseConfig* Config) const
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
			const FVector ToTarget = Context.GetOrigin() - Instigator->GetActorLocation();
			if (!ToTarget.IsNearlyZero())
			{
				return ToTarget.GetSafeNormal();
			}
		}
		return Instigator->GetActorForwardVector();
	}

	case EMoveDirectionSource::TowardTarget:
	{
		const FGameplayEffectContextHandle& Context = GESpec.GetEffectContext();
		if (const FHitResult* const Hit = Context.GetHitResult())
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

			const FVector ToTarget = TargetLocation - Instigator->GetActorLocation();
			if (!ToTarget.IsNearlyZero())
			{
				return ToTarget.GetSafeNormal();
			}
		}
		return Instigator->GetActorForwardVector();
	}
	}

	return Instigator->GetActorForwardVector();
}

void UMoveBaseGEC::HandleWallHit(AActor* Instigator, const FHitResult& Hit, const UMoveBaseConfig* Config, const FGameplayEffectSpec& GESpec) const
{
	if (!IsValid(Instigator) || !IsValid(Config) || !Config->bDetectWallHit)
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

void UMoveBaseGEC::SnapToGround(FVector& InOutLocation, const UMoveBaseConfig* Config, const AActor* Instigator) const
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
	const FVector TraceEnd = InOutLocation - FVector(0.0f, 0.0f, Config->GroundTraceDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Instigator);

	if (World->LineTraceSingleByChannel(FloorHit, InOutLocation, TraceEnd, Config->GroundTraceChannel, QueryParams))
	{
		InOutLocation.Z = FloorHit.Location.Z;
	}
}

void UMoveBaseGEC::ExecuteMoveCue(const USkillNiagaraSpawnConfig* VfxConfig, const FGameplayEffectSpec& GESpec, AActor* Instigator, const FVector& Location) const
{
	if (!IsValid(VfxConfig) || !VfxConfig->CueTag.IsValid() || !IsValid(Instigator))
	{
		return;
	}

	UAbilitySystemComponent* const InstigatorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Instigator);
	if (!IsValid(InstigatorASC))
	{
		return;
	}

	const FGameplayEffectContextHandle& ContextHandle = GESpec.GetEffectContext();

	FGameplayCueParameters Params(GESpec);
	Params.OriginalTag = VfxConfig->CueTag;
	Params.Instigator = ContextHandle.GetInstigator();
	Params.EffectCauser = Instigator;
	Params.Location = Location;
	Params.SourceObject = VfxConfig;
	
	{
		FScopedPredictionWindow ForcedWindow(InstigatorASC, FPredictionKey(), false);
		InstigatorASC->ExecuteGameplayCue(VfxConfig->CueTag, Params);
	}
}

void UMoveBaseGEC::AddMovingCue(const USkillNiagaraSpawnConfig* VfxConfig, const FGameplayEffectSpec& GESpec, AActor* Instigator, const FVector& Direction, float Speed) const
{
	if (!IsValid(VfxConfig) || !VfxConfig->CueTag.IsValid() || !IsValid(Instigator))
	{
		return;
	}

	UAbilitySystemComponent* const InstigatorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Instigator);
	if (!IsValid(InstigatorASC))
	{
		return;
	}

	const FGameplayEffectContextHandle& ContextHandle = GESpec.GetEffectContext();
	FGameplayCueParameters Params(GESpec);
	Params.OriginalTag = VfxConfig->CueTag;
	Params.Instigator = ContextHandle.GetInstigator();
	Params.EffectCauser = Instigator;
	Params.Location = Instigator->GetActorLocation();
	Params.Normal = Direction;      // 이동 방향 전달
	Params.RawMagnitude = Speed;    // 이동 속도 전달
	Params.SourceObject = VfxConfig;

	{
		FScopedPredictionWindow ForcedWindow(InstigatorASC, FPredictionKey(), false);
		InstigatorASC->AddGameplayCue(VfxConfig->CueTag, Params);
	}
}

void UMoveBaseGEC::RemoveMovingCue(const USkillNiagaraSpawnConfig* VfxConfig, AActor* Instigator) const
{
	if (!IsValid(VfxConfig) || !VfxConfig->CueTag.IsValid() || !IsValid(Instigator))
	{
		return;
	}

	UAbilitySystemComponent* const InstigatorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Instigator);
	if (!IsValid(InstigatorASC))
	{
		return;
	}

	{
		FScopedPredictionWindow ForcedWindow(InstigatorASC, FPredictionKey(), false);
		InstigatorASC->RemoveGameplayCue(VfxConfig->CueTag);
	}
}

void UMoveBaseGEC::AdjustActiveMontageRate(ACharacter* Character, float MoveDuration, const UMoveBaseConfig* Config) const
{
	if (!IsValid(Character) || !IsValid(Config) || !Config->bAdjustMontageRate)
	{
		return;
	}

	if (MoveDuration <= 0.0f)
	{
		return;
	}

	UAnimInstance* const AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!IsValid(AnimInstance))
	{
		return;
	}

	FAnimMontageInstance* const MontageInstance = AnimInstance->GetActiveMontageInstance();
	if (MontageInstance == nullptr || !IsValid(MontageInstance->Montage))
	{
		return;
	}

	// 현재 재생 위치를 고려하여 남은 시간 계산
	const float CurrentPosition = MontageInstance->GetPosition();
	const float MontageLength = MontageInstance->Montage->GetPlayLength();
	const float RemainingLength = MontageLength - CurrentPosition;

	if (RemainingLength <= 0.0f)
	{
		return;
	}

	// 실제 이동 시간에 맞춰 재생 속도 계산 (남은 길이 / 이동 시간)
	const float NewRate = FMath::Clamp(RemainingLength / MoveDuration, Config->MinPlayRate, Config->MaxPlayRate);
	MontageInstance->SetPlayRate(NewRate);
}

void UMoveBaseGEC::SetPawnCollisionIgnore(ACharacter* Character, bool bIgnore) const
{
	if (!IsValid(Character))
	{
		return;
	}

	UCapsuleComponent* const Capsule = Character->GetCapsuleComponent();
	if (!IsValid(Capsule))
	{
		return;
	}

	Capsule->SetCollisionResponseToChannel(ECC_Pawn, bIgnore ? ECR_Ignore : ECR_Block);
}
